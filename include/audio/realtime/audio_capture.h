#pragma once
#define NOMINMAX
#include <audio/audio_internal.h>
#include <containers/ring_buffer.h>
#include <thread>
#include <functional>
#include <algorithm> 

namespace audio { 
  namespace detail {
    auto make_processor_float = [](uint16_t srcCh, UINT32 numFrames, void* pData, containers::ring_buffer<float>& outBuf) {
      const float* src = reinterpret_cast<const float*>(pData);
      for (UINT32 f = 0; f < numFrames; ++f) {
        float mix = 0.0f;
        size_t base = static_cast<size_t>(f) * srcCh;
        for (uint16_t c = 0; c < srcCh; ++c) {
          mix += src[base + c];
        }
        mix /= static_cast<float>(srcCh);
        outBuf.push(mix);
      }
    };

    template<typename IntT>
    auto make_processor_int = [](uint16_t srcCh, UINT32 numFrames, void* pData, containers::ring_buffer<float>& outBuf) {
      const IntT* src = reinterpret_cast<const IntT*>(pData);
      constexpr double div = static_cast<double>(std::numeric_limits<IntT>::max());
      for (UINT32 f = 0; f < numFrames; ++f) {
        double mix = 0.0;
        size_t base = static_cast<size_t>(f) * srcCh;
        for (uint16_t c = 0; c < srcCh; ++c) {
          mix += static_cast<double>(src[base + c]) / div;
        }
        mix /= srcCh;
        float out = static_cast<float>(mix);
        outBuf.push(out);
      }
    };
  } // namespace detail
  class AudioCapture {
  public:
    bool initialize(size_t duration_sec = 5);
    void start_capture();
    void get_captured_buffer(containers::ring_buffer<float>& out) const noexcept(noexcept(std::declval<containers::ring_buffer<float>&>().get_interval(std::declval<size_t>(), std::declval<size_t>(), std::declval<containers::ring_buffer<float>&>())));
    _NODISCARD inline containers::ring_buffer<float>& get_buffer() noexcept { return captured_buffer; }
    inline void set_callback_function(std::function<void(uint16_t, UINT32, void*, containers::ring_buffer<float>&)> f) { m_f = f; }
    void stop();
    void wait_for_completion();
    void clear_buffer() noexcept;
    void shutdown();
  private:
    bool capture_loopback_buffer();

  public:
    AudioCapture(const AudioCapture&) = delete;
    AudioCapture& operator=(const AudioCapture&) = delete;
    static AudioCapture& Get() {
      static AudioCapture ac;
      return ac;
    }

  private:
    ~AudioCapture();
    AudioCapture();
  
  private:
    ComPtr<IAudioClient> m_audioClient;
    ComPtr<IAudioCaptureClient> m_captureClient;
    HANDLE hEvent = nullptr;
    uint32_t m_srcRate;
    uint16_t m_srcCh;
    uint16_t m_srcBits;
    bool bFloat;
    //TODO: it's better to make it an integer
    size_t m_durationSec;

    std::thread m_workerThread;
    std::atomic<bool> m_stopFlag;

    std::function<void(uint16_t, UINT32, void*, containers::ring_buffer<float>&)> m_f;
    containers::ring_buffer<float> captured_buffer;
  };
}