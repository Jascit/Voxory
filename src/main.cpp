
#include <whisper.h>
#define NOMINMAX
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <vector>
#include <windows.h>
  ///
#include <containers/chunk_buffer.h>
#include <audio/audio_internal.h>
#include <audio/realtime/audio_capture.h>
//
//  static std::string to_timestamp(int64_t t, bool comma = false) {
//  int64_t msec = t * 10;
//  int64_t hr = msec / (1000 * 60 * 60);
//  msec = msec - hr * (1000 * 60 * 60);
//  int64_t min = msec / (1000 * 60);
//  msec = msec - min * (1000 * 60);
//  int64_t sec = msec / 1000;
//  msec = msec - sec * 1000;
//
//  char buf[32];
//  snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int)hr, (int)min, (int)sec, comma ? "," : ".", (int)msec);
//
//  return std::string(buf);
//}
//
//// ---------- Main: producer/consumer threads ----------
//int main() {
//  SetConsoleOutputCP(CP_UTF8);
//  ggml_backend_load_all();
//
//  // config
//  const std::string MODEL_PATH = "C:\\CPP\\transcripter\\dependencies\\whisper\\models\\ggml-large-v3-turbo.bin";
//  const std::string OUT_TEXT = "transcript.txt";
//  const bool USE_GPU = true;
//  const bool FLASH_ATTN = true;
//  const int THREADS = std::min(4u, std::thread::hardware_concurrency());
//
//  // whisper init (context shared Ч consumer uses it for inference)
//  struct whisper_context_params cparams = whisper_context_default_params();
//  cparams.use_gpu = USE_GPU;
//  cparams.flash_attn = FLASH_ATTN;
//
//  std::cerr << "[info] initializing model '" << MODEL_PATH << "' ...\n";
//  struct whisper_context* ctx = whisper_init_from_file_with_params(MODEL_PATH.c_str(), cparams);
//  if (!ctx) {
//    std::cerr << "[error] failed to init model\n";
//    return 1;
//  }
//
//  const size_t sample_rate = WHISPER_SAMPLE_RATE; // 16000
//  const size_t chunk_seconds = 30;
//  const size_t chunk_samples = sample_rate * chunk_seconds;
//  const size_t capacity_seconds = chunk_seconds * 2; // total buffer seconds
//  const size_t capacity_slots = std::max((size_t)4, capacity_seconds / chunk_seconds);
//
//  ChunkRing ring(capacity_slots, chunk_samples);
//  std::atomic<bool> stop_flag(false);
//
//  // consumer thread: no mutex, busy wait with small sleep
//  std::thread consumer([&]() {
//    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
//    wparams.print_progress = false;
//    wparams.print_realtime = false;
//    wparams.print_timestamps = true;
//    wparams.translate = false;
//    wparams.single_segment = true;
//    wparams.n_threads = THREADS;
//    wparams.language = "en";
//
//    size_t out_count = 0;
//    while (!stop_flag.load(std::memory_order_relaxed)) {
//      const float* ptr = ring.try_pop_chunk(out_count);
//      if (ptr == nullptr) {
//        // no data: busy-wait with tiny sleep
//        std::this_thread::sleep_for(std::chrono::milliseconds(1));
//        continue;
//      }
//      // ptr points to chunk_samples floats; do NOT copy
//      // run inference directly on ptr
//      int rc = whisper_full(ctx, wparams, (float*)ptr, (int)out_count);
//      if (rc != 0) {
//        std::cerr << "[error] whisper_full failed: " << rc << "\n";
//        continue;
//      }
//
//      const int n_segments = whisper_full_n_segments(ctx);
//      std::string out_all;
//      for (int i = 0; i < n_segments; ++i) {
//        const char* text = whisper_full_get_segment_text(ctx, i);
//        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
//        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
//        std::string line = "[" + to_timestamp(t0) + " --> " + to_timestamp(t1) + "] " + text + "\n";
//        out_all += line;
//        std::cout << line;
//      }
//      if (!out_all.empty()) {
//        std::ofstream fout(OUT_TEXT, std::ios::app | std::ios::binary);
//        if (fout) fout.write(out_all.data(), out_all.size());
//      }
//    }
//    std::cerr << "[info] consumer exiting\n";
//    });
//
//  // producer thread: capture 1s -> parse -> copy into ring slot
//  std::thread listener([&]() {
//    capture_loopback_5s_wav_buffer(ring);
//    });
//
//  std::thread producer([&]() {
//    while (!stop_flag.load(std::memory_order_relaxed)) {
//      std::vector<float> pcmf32;
//      int sr = 0, ch = 0;
//      if (!read_wav_pcm16_from_buffer(wavbuf, pcmf32, sr, ch)) {
//        std::cerr << "[error] parse wav failed\n";
//        std::this_thread::sleep_for(std::chrono::milliseconds(200));
//        continue;
//      }
//      if (sr != (int)sample_rate) {
//        std::cerr << "[warn] sample rate mismatch: " << sr << " expected " << sample_rate << "\n";
//        // TODO: resample here if necessary; for now skip
//        continue;
//      }
//      if (ch != 1) {
//        std::cerr << "[warn] channels != 1: " << ch << "\n";
//        continue;
//      }
//
//      // push chunk (copy from pcmf32 into ring slot). This is the only memcpy.
//      bool ok = ring.push_chunk(pcmf32.data(), stop_flag);
//      if (!ok) break;
//    }
//    std::cerr << "[info] producer exiting\n";
//    });
//  // run until user presses Enter
//  std::vector<uint8_t> a;
//  capture_loopback_5s_wav_buffer(5);
//  std::cerr << "[info] running. Press Enter to stop...\n";
//  std::string dummy;
//  std::getline(std::cin, dummy);
//  stop_flag.store(true);
//
//  consumer.join();
//  //  producer.join();
//
//  whisper_print_timings(ctx);
//  whisper_free(ctx);
//
//  std::cerr << "[info] exited\n";
//  return 0;
//}


int main() {
  std::cout << "Hello, World!" << std::endl;
  audio::AudioCapture& ac = audio::AudioCapture::Get();
  ac.initialize(1); // 5 seconds buffer
  ac.start_capture();
  std::this_thread::sleep_for(std::chrono::milliseconds(4990));
  containers::ring_buffer<float> captured_mono;
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
  std::cout << "time taken to get buffer(ring): " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << " ns\n";
  captured_mono.reserve(44100);
  ac.get_captured_buffer(captured_mono);
  std::vector<float> source(44100, 0.5f);

  std::vector<float> dest;
  dest.resize(source.size());

  auto start1 = std::chrono::steady_clock::now();

  // ≈мул€ц≥€ твого get_captured_buffer: коп≥юЇмо з source у dest
  std::memcpy(dest.data(), source.data(), source.size() * sizeof(float));

  auto end1 = std::chrono::steady_clock::now();

  std::cout
    << "time taken to copy vector buffer: "
    << std::chrono::duration_cast<std::chrono::nanoseconds>(end1 - start1).count()
    << " ns\n";
  ac.shutdown();
  return 0;
}

class some_shit {
  some_shit() {
    data = static_cast<size_t*>(malloc(sizeof(size_t) * 100));
  }
  ~some_shit() {
    free(data);
  }
private:
  size_t* data;
};
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
