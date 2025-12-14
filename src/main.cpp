/*
  MVP: Windows WASAPI Loopback ? Whisper real-time transcription

  Purpose:
  --------
  Minimal end-to-end prototype that captures system audio
  (what the user hears) via WASAPI loopback and feeds it
  into Whisper for near real-time speech-to-text.

  What this code does:
  --------------------
  - Initializes COM and WASAPI in shared loopback mode
  - Captures audio from the default render device
  - Downmixes multi-channel audio to mono
  - Resamples device sample rate to WHISPER_SAMPLE_RATE
  - Collects fixed-size audio chunks (step_ms / length_ms)
  - Runs Whisper inference continuously
  - Prints recognized text to stdout

  Design notes:
  -------------
  - This is an MVP / proof-of-concept, not production code
  - Error handling is minimal and mostly fail-fast
  - Resampling is naive (accumulator-based, no filtering)
  - No advanced VAD, buffering strategy, or latency tuning
  - Audio pipeline prioritizes simplicity over correctness

  Assumptions:
  ------------
  - Windows only
  - Default audio output device
  - Whisper model already downloaded
  - GPU available and enabled (if supported)

  Intended use:
  -------------
  - Rapid prototyping
  - Experiments with real-time transcription
  - Base for a cleaner, modular audio + inference pipeline

  Not intended for:
  -----------------
  - Long-running production services
  - High-quality audio processing
  - Accurate timing or lip-sync use cases
*/

#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <comdef.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "whisper.h"

static const int n_threads = std::min(4, (int)std::thread::hardware_concurrency());
static const int step_ms = 15000;
static const int length_ms = 15000;
static const int keep_ms = 0;

//
// Minimal WASAPI loopback capture helper
//
class WasapiLoopback {
public:
  WasapiLoopback() : pEnumerator(nullptr), pDevice(nullptr), pAudioClient(nullptr), pCaptureClient(nullptr), pwfx(nullptr), running(false) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  }

  ~WasapiLoopback() {
    stop();
    if (pCaptureClient) pCaptureClient->Release();
    if (pAudioClient) pAudioClient->Release();
    if (pDevice) pDevice->Release();
    if (pEnumerator) pEnumerator->Release();
    if (pwfx) CoTaskMemFree(pwfx);
    CoUninitialize();
  }

  bool init(int target_sample_rate) {
    // get default render device
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
      __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
    if (FAILED(hr) || !pEnumerator) {
      fprintf(stderr, "WASAPI: failed to create MMDeviceEnumerator: 0x%08x\n", hr);
      return false;
    }

    hr = pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    if (FAILED(hr) || !pDevice) {
      fprintf(stderr, "WASAPI: failed to get default render endpoint: 0x%08x\n", hr);
      return false;
    }

    hr = pDevice->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&pAudioClient);
    if (FAILED(hr) || !pAudioClient) {
      fprintf(stderr, "WASAPI: failed to activate IAudioClient: 0x%08x\n", hr);
      return false;
    }

    // get mix format
    hr = pAudioClient->GetMixFormat(&pwfx);
    if (FAILED(hr) || !pwfx) {
      fprintf(stderr, "WASAPI: GetMixFormat failed: 0x%08x\n", hr);
      return false;
    }

    const REFERENCE_TIME hnsRequestedDuration = 10000000; // 1 second for buffer (safe)
    DWORD streamFlags = AUDCLNT_STREAMFLAGS_LOOPBACK;

    hr = pAudioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, hnsRequestedDuration, 0, pwfx, nullptr);
    if (FAILED(hr)) {
      fprintf(stderr, "WASAPI: IAudioClient::Initialize failed: 0x%08x\n", hr);
      return false;
    }

    hr = pAudioClient->GetService(__uuidof(IAudioCaptureClient), (void**)&pCaptureClient);
    if (FAILED(hr) || !pCaptureClient) {
      fprintf(stderr, "WASAPI: GetService(IAudioCaptureClient) failed: 0x%08x\n", hr);
      return false;
    }

    // store some format info
    channels = pwfx->nChannels;
    device_sample_rate = pwfx->nSamplesPerSec;
    bytes_per_sample = pwfx->wBitsPerSample / 8;

    return true;
  }

  bool start() {
    if (!pAudioClient) return false;
    HRESULT hr = pAudioClient->Start();
    if (FAILED(hr)) {
      fprintf(stderr, "WASAPI: Start failed: 0x%08x\n", hr);
      return false;
    }
    running = true;
    return true;
  }

  void stop() {
    if (pAudioClient && running) {
      pAudioClient->Stop();
      running = false;
    }
  }

  bool get(int ms, std::vector<float>& out) {
    if (!pCaptureClient || !pwfx) return false;

    const uint32_t wanted_frames = (uint32_t)((int64_t)ms * WHISPER_SAMPLE_RATE / 1000);
    out.clear();
    out.reserve(wanted_frames);

    uint32_t collected = 0;

    int loops = 0;

    while (collected < wanted_frames) {
      UINT32 packetFrames = 0;
      HRESULT hr = pCaptureClient->GetNextPacketSize(&packetFrames);
      if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: GetNextPacketSize failed: 0x%08x\n", hr);
        return false;
      }

      if (packetFrames == 0) {
        // no data yet: sleep small bit
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }

      BYTE* pData;
      UINT32 framesAvailable;
      DWORD flags;
      hr = pCaptureClient->GetBuffer(&pData, &framesAvailable, &flags, nullptr, nullptr);
      if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: GetBuffer failed: 0x%08x\n", hr);
        return false;
      }

      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        for (UINT32 f = 0; f < framesAvailable && collected < wanted_frames; ++f) {
          out.push_back(0.0f);
          ++collected;
        }
      }
      else {
        if (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
          (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE && ((WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {

          const float* src = reinterpret_cast<const float*>(pData);
          for (UINT32 f = 0; f < framesAvailable; ++f) {
            float sample = 0.0f;
            for (int c = 0; c < channels; ++c) {
              sample += src[f * channels + c];
            }
            sample /= (float)channels;
            append_sample_resample(sample, device_sample_rate, out, collected, wanted_frames);
          }
        }
        else if (pwfx->wFormatTag == WAVE_FORMAT_PCM) {
          if (bytes_per_sample == 2) {
            const int16_t* src = reinterpret_cast<const int16_t*>(pData);
            for (UINT32 f = 0; f < framesAvailable; ++f) {
              float sample = 0.0f;
              for (int c = 0; c < channels; ++c) {
                sample += src[f * channels + c] / 32768.0f;
              }
              sample /= (float)channels;
              append_sample_resample(sample, device_sample_rate, out, collected, wanted_frames);
            }
          }
          else {
            for (UINT32 f = 0; f < framesAvailable && collected < wanted_frames; ++f) {
              out.push_back(0.0f);
              ++collected;
            }
          }
        }
        else {
          for (UINT32 f = 0; f < framesAvailable && collected < wanted_frames; ++f) {
            out.push_back(0.0f);
            ++collected;
          }
        }
      }

      hr = pCaptureClient->ReleaseBuffer(framesAvailable);
      if (FAILED(hr)) {
        fprintf(stderr, "WASAPI: ReleaseBuffer failed: 0x%08x\n", hr);
        return false;
      }
    }

    while (collected < wanted_frames) {
      out.push_back(0.0f);
      ++collected;
    }

    return true;
  }

private:
  IMMDeviceEnumerator* pEnumerator;
  IMMDevice* pDevice;
  IAudioClient* pAudioClient;
  IAudioCaptureClient* pCaptureClient;
  WAVEFORMATEX* pwfx;
  UINT32 channels;
  UINT32 device_sample_rate;
  UINT32 bytes_per_sample;
  bool running;

  void append_sample_resample(float sample, uint32_t dev_rate, std::vector<float>& out, uint32_t& collected, uint32_t wanted_frames) {
    static double acc = 0.0;
    const double ratio = (double)dev_rate / (double)WHISPER_SAMPLE_RATE;
    if (dev_rate == WHISPER_SAMPLE_RATE) {
      if (collected < wanted_frames) { out.push_back(sample); ++collected; }
      return;
    }
    acc += 1.0 / ratio;
    if (acc >= 1.0) {
      if (collected < wanted_frames) { out.push_back(sample); ++collected; }
      acc -= 1.0;
    }
    else {
      //skip
    }
  }
};

int main() {
  ggml_backend_load_all();

  const int n_samples_step = (1e-3 * step_ms) * WHISPER_SAMPLE_RATE;
  const int n_samples_len = (1e-3 * length_ms) * WHISPER_SAMPLE_RATE;
  const int n_samples_keep = (1e-3 * keep_ms) * WHISPER_SAMPLE_RATE;
  const int n_samples_30s = (1e-3 * 30000.0) * WHISPER_SAMPLE_RATE;

  const bool use_vad = n_samples_step <= 0;
  const int n_new_line = !use_vad ? std::max(1, length_ms / step_ms - 1) : 1;

  std::string model_path = "C:\\CPP\\Voxory\\dependencies\\whisper\\models\\ggml-large-v3-turbo.bin";

  struct whisper_context_params cparams = whisper_context_default_params();
  cparams.use_gpu = true;
  cparams.flash_attn = true;

  struct whisper_context* ctx = whisper_init_from_file_with_params(model_path.c_str(), cparams);
  if (ctx == nullptr) {
    fprintf(stderr, "error: failed to initialize whisper context\n");
    return 2;
  }

  std::vector<float> pcmf32(n_samples_30s, 0.0f);
  std::vector<float> pcmf32_old;
  std::vector<float> pcmf32_new(n_samples_30s, 0.0f);
  std::vector<whisper_token> prompt_tokens;

  WasapiLoopback wasapi;
  if (!wasapi.init(WHISPER_SAMPLE_RATE)) {
    fprintf(stderr, "Failed to init WASAPI loopback\n");
    return 1;
  }
  if (!wasapi.start()) {
    fprintf(stderr, "Failed to start WASAPI capture\n");
    return 1;
  }

  printf("[Start capturing loopback — what the user hears]\n");
  fflush(stdout);

  int n_iter = 0;
  bool is_running = true;

  while (is_running) {
    if (!use_vad) {
      if (!wasapi.get(step_ms, pcmf32_new)) {
        fprintf(stderr, "WASAPI get failed\n");
        break;
      }

      const int n_samples_new = pcmf32_new.size();
      const int n_samples_take = std::min((int)pcmf32_old.size(), std::max(0, n_samples_keep + n_samples_len - n_samples_new));

      pcmf32.resize(n_samples_new + n_samples_take);
      for (int i = 0; i < n_samples_take; ++i) {
        pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
      }
      memcpy(pcmf32.data() + n_samples_take, pcmf32_new.data(), n_samples_new * sizeof(float));
      pcmf32_old = pcmf32;
    }
    else {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
    wparams.print_progress = false;
    wparams.print_special = false;
    wparams.print_realtime = false;
    wparams.print_timestamps = false;
    wparams.translate = false;
    wparams.single_segment = !use_vad;
    wparams.max_tokens = 0;
    wparams.language = "en";
    wparams.n_threads = n_threads;

    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
      fprintf(stderr, "whisper_full() failed\n");
      break;
    }

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
      const char* text = whisper_full_get_segment_text(ctx, i);
      printf("%s", text);
      fflush(stdout);
    }
    printf("\n");
    ++n_iter;

    if (!use_vad && (n_iter % n_new_line) == 0) {
      pcmf32_old = std::vector<float>(pcmf32.end() - n_samples_keep, pcmf32.end());
    }
  }

  wasapi.stop();
  whisper_free(ctx);
  return 0;
}
