#include <audio/realtime/audio_capture.h>

AudioCapture::~AudioCapture() {
  if (audioClient) {
    audioClient->Stop();
  }
  if (hEvent) {
    CloseHandle(hEvent);
    hEvent = nullptr;
  }
  if (hEvent) CloseHandle(hEvent);
  CoTaskMemFree(pwfx);
  CoUninitialize();
}
bool AudioCapture::initialize() {
  constexpr uint32_t TARGET_SR = 16000;
  constexpr uint16_t TARGET_CH = 1;
  constexpr uint16_t TARGET_BITS = 16;
  double DURATION_SEC = duration_sec;

  HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hr)) {
    std::cerr << "CoInitializeEx failed: 0x" << std::hex << hr << std::dec << "\n";
    return false;
  }

  // Enumerator
  ComPtr<IMMDeviceEnumerator> enumerator;
  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&enumerator));
  if (FAILED(hr)) { std::cerr << "CoCreateInstance MMDeviceEnumerator failed\n"; CoUninitialize(); return false; }

  // Default render device (we capture its render stream via loopback)
  ComPtr<IMMDevice> device;
  hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
  if (FAILED(hr)) { std::cerr << "GetDefaultAudioEndpoint failed\n"; CoUninitialize(); return false; }

  // Activate IAudioClient
  hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(audioClient.GetAddressOf()));
  if (FAILED(hr)) { std::cerr << "Device Activate failed: 0x" << std::hex << hr << std::dec << "\n"; CoUninitialize(); return false; }

  // Get mix format
  WAVEFORMATEX* pwfx = nullptr;
  hr = audioClient->GetMixFormat(&pwfx);
  if (FAILED(hr) || pwfx == nullptr) { std::cerr << "GetMixFormat failed\n"; CoUninitialize(); return false; }

  // Determine details (handle WAVEFORMATEXTENSIBLE)
  src_rate = pwfx->nSamplesPerSec;
  src_ch = pwfx->nChannels;
  src_bits = pwfx->wBitsPerSample;
  is_float = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);

  if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    WAVEFORMATEXTENSIBLE* pExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
    src_ch = pExt->Format.nChannels;
    src_rate = pExt->Format.nSamplesPerSec;
    // Determine if float via SubFormat GUID
    // Compare GUIDs
    if (IsEqualGUID(pExt->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
      is_float = true;
      src_bits = 32;
    }
    else {
      // integer PCM subtype (KSDATAFORMAT_SUBTYPE_PCM)
      is_float = false;
      // If validBitsPerSample given, use it; otherwise use Format.wBitsPerSample
      src_bits = (pExt->Samples.wValidBitsPerSample != 0) ? pExt->Samples.wValidBitsPerSample : pExt->Format.wBitsPerSample;
    }
  }

  // Initialize with event callback + loopback
  hEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  if (!hEvent) {
    std::cerr << "CreateEvent failed\n";
    CoTaskMemFree(pwfx);
    CoUninitialize();
    return false;
  }

  REFERENCE_TIME bufferDuration = static_cast<REFERENCE_TIME>(0.5 * 10000000.0); // 0.5s buffer - fine for loopback
  DWORD flags = AUDCLNT_STREAMFLAGS_LOOPBACK | AUDCLNT_STREAMFLAGS_EVENTCALLBACK;

  hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufferDuration, 0, pwfx, nullptr);
  if (FAILED(hr)) {
    // fallback: try without eventcallback (older drivers)
    hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx, nullptr);
    if (FAILED(hr)) {
      std::cerr << "AudioClient Initialize failed: 0x" << std::hex << hr << std::dec << "\n";
      CloseHandle(hEvent);
      CoTaskMemFree(pwfx);
      CoUninitialize();
      return false;
    }
  }
  else {
    // success with event callback: set handle
    hr = audioClient->SetEventHandle(hEvent);
    if (FAILED(hr)) {
      std::cerr << "SetEventHandle failed: 0x" << std::hex << hr << std::dec << "\n";
      // not fatal; we'll continue with polling fallback
    }
  }
  // Get IAudioCaptureClient
  hr = audioClient->GetService(IID_PPV_ARGS(&captureClient));
  if (FAILED(hr)) { std::cerr << "GetService IAudioCaptureClient failed\n"; CloseHandle(hEvent); CoTaskMemFree(pwfx); CoUninitialize(); return false; }
  return true;
};


bool AudioCapture::capture_loopback_buffer(double duration_sec) {

  // Start capture & boost audio thread priority
  hr = audioClient->Start();
  if (FAILED(hr)) { std::cerr << "audioClient Start failed\n"; CloseHandle(hEvent); CoTaskMemFree(pwfx); CoUninitialize(); return false; }

  // Attempt to boost via Avrt
  DWORD mmTaskIndex = 0;
  HANDLE hAvTask = AvSetMmThreadCharacteristicsA("Pro Audio", &mmTaskIndex);
  if (hAvTask == NULL) {
    DWORD err = GetLastError();
    std::cerr << "[warn] AvSetMmThreadCharacteristicsA failed (err=" << err << "), using SetThreadPriority fallback\n";
    // fallback: try to increase thread priority
    BOOL ok = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    if (!ok) {
      std::cerr << "[warn] SetThreadPriority failed (err=" << GetLastError() << ")\n";
    }
  }
  else {
    // Optional: set Avrt priority level if desired
    if (!AvSetMmThreadPriority(hAvTask, AVRT_PRIORITY_HIGH)) {
      std::cerr << "[warn] AvSetMmThreadPriority failed (err=" << GetLastError() << ")\n";
    }
  }

  // collect into float mono vector
  std::vector<float> captured_mono;
  captured_mono.reserve(static_cast<size_t>(src_rate * DURATION_SEC * 1.1) + 256);

  const size_t want_frames = static_cast<size_t>(std::ceil(src_rate * DURATION_SEC));
  bool done = false;

  while (!done) {
    // wait for event (if available) or poll
    DWORD waitMs = 2000;
    DWORD waitRes = WAIT_OBJECT_0;
    if (hEvent) {
      waitRes = WaitForSingleObject(hEvent, waitMs);
      if (waitRes == WAIT_TIMEOUT) {
        // nothing arrived within timeout - continue loop to try again
        // but check if we already collected enough
        if (captured_mono.size() >= want_frames) break;
        continue;
      }
    }
    else {
      Sleep(5);
    }

    // Read all available packets
    UINT32 packetFrames = 0;
    HRESULT hr2 = captureClient->GetNextPacketSize(&packetFrames);
    if (FAILED(hr2)) {
      std::cerr << "GetNextPacketSize failed\n";
      break;
    }

    while (packetFrames > 0) {
      BYTE* pData = nullptr;
      UINT32 numFrames = 0;
      DWORD flags = 0;
      hr2 = captureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
      if (FAILED(hr2)) {
        std::cerr << "GetBuffer failed\n";
        done = true;
        break;
      }

      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        // silent: push zeros
        for (UINT32 f = 0; f < numFrames; ++f) {
          captured_mono.push_back(0.0f);
        }
      }
      else {
        // Interpret buffer according to format
        if (is_float) {
          float* src = reinterpret_cast<float*>(pData);
          for (UINT32 f = 0; f < numFrames; ++f) {
            double mix = 0.0;
            for (uint16_t c = 0; c < src_ch; ++c) {
              mix += src[(size_t)f * src_ch + c];
            }
            mix /= src_ch;
            captured_mono.push_back(static_cast<float>(mix));
          }
        }
        else {
          // integer PCM
          if (src_bits == 16) {
            int16_t* src = reinterpret_cast<int16_t*>(pData);
            for (UINT32 f = 0; f < numFrames; ++f) {
              double mix = 0.0;
              for (uint16_t c = 0; c < src_ch; ++c) {
                mix += static_cast<double>(src[(size_t)f * src_ch + c]) / 32768.0;
              }
              mix /= src_ch;
              captured_mono.push_back(static_cast<float>(mix));
            }
          }
          else if (src_bits == 32) {
            int32_t* src = reinterpret_cast<int32_t*>(pData);
            for (UINT32 f = 0; f < numFrames; ++f) {
              double mix = 0.0;
              for (uint16_t c = 0; c < src_ch; ++c) {
                mix += static_cast<double>(src[(size_t)f * src_ch + c]) / 2147483648.0;
              }
              mix /= src_ch;
              captured_mono.push_back(static_cast<float>(mix));
            }
          }
          else {
            std::cerr << "Unsupported integer bit depth: " << src_bits << "\n";
            // release and abort
            captureClient->ReleaseBuffer(numFrames);
            done = true;
            break;
          }
        }
      }

      // Release buffer with the exact numFrames
      hr2 = captureClient->ReleaseBuffer(numFrames);
      if (FAILED(hr2)) {
        std::cerr << "ReleaseBuffer failed\n";
        done = true;
        break;
      }

      // check length
      if (captured_mono.size() >= want_frames) { done = true; break; }

      // get next packet size
      hr2 = captureClient->GetNextPacketSize(&packetFrames);
      if (FAILED(hr2)) { std::cerr << "GetNextPacketSize failed\n"; done = true; break; }
    } // while packetFrames
  } // while !done

  if (hAvTask != NULL) {
    BOOL rv = AvRevertMmThreadCharacteristics(hAvTask);
    if (!rv) {
      std::cerr << "[warn] AvRevertMmThreadCharacteristics failed (err=" << GetLastError() << ")\n";
      // don't crash — just continue cleanup
    }
  }
  else {
    // If we used SetThreadPriority fallback, optionally reset it:
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
  }

  ///////////////
  // Resample to TARGET_SR
  //std::vector<float> resampled = resample_linear(captured_mono, src_rate, TARGET_SR);

  // Ensure exact length TARGET_SR * DURATION_SEC
  //size_t want_samples = static_cast<size_t>(std::round(TARGET_SR * DURATION_SEC));
  //if (resampled.size() < want_samples) resampled.resize(want_samples, 0.0f);
  //if (resampled.size() > want_samples) resampled.resize(want_samples);

  //std::vector<int16_t> pcm16;
  //pcm16.reserve(resampled.size());
  //for (float v : resampled) pcm16.push_back(f32_to_s16_clamped(v));

  return true;
}