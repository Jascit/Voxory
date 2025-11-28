#include <audio/realtime/audio_capture.h>
using namespace audio;
AudioCapture::AudioCapture() : bFloat(false), m_durationSec(0), m_srcBits(0), m_srcCh(0), m_srcRate(0), m_stopFlag{ false } {}
AudioCapture::~AudioCapture() {
  shutdown();
}

bool AudioCapture::initialize(size_t duration_sec) {
  m_durationSec = duration_sec;

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
  hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(m_audioClient.GetAddressOf()));
  if (FAILED(hr)) { std::cerr << "Device Activate failed: 0x" << std::hex << hr << std::dec << "\n"; CoUninitialize(); return false; }

  // Get mix format
  WAVEFORMATEX* pwfx = nullptr;
  hr = m_audioClient->GetMixFormat(&pwfx);
  if (FAILED(hr) || pwfx == nullptr) { std::cerr << "GetMixFormat failed\n"; CoUninitialize(); return false; }

  // Determine details (handle WAVEFORMATEXTENSIBLE)
  m_srcRate = pwfx->nSamplesPerSec;
  m_srcCh = pwfx->nChannels;
  m_srcBits = pwfx->wBitsPerSample;
  bFloat = (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);

  if (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
    WAVEFORMATEXTENSIBLE* pExt = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(pwfx);
    m_srcCh = pExt->Format.nChannels;
    m_srcRate = pExt->Format.nSamplesPerSec;
    // Determine if float via SubFormat GUID
    // Compare GUIDs
    if (IsEqualGUID(pExt->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
      bFloat = true;
      m_srcBits = 32;
    }
    else {
      // integer PCM subtype (KSDATAFORMAT_SUBTYPE_PCM)
      bFloat = false;
      // If validBitsPerSample given, use it; otherwise use Format.wBitsPerSample
      m_srcBits = (pExt->Samples.wValidBitsPerSample != 0) ? pExt->Samples.wValidBitsPerSample : pExt->Format.wBitsPerSample;
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

  hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, bufferDuration, 0, pwfx, nullptr);
  if (FAILED(hr)) {
    // fallback: try without eventcallback (older drivers)
    hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 0, 0, pwfx, nullptr);
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
    hr = m_audioClient->SetEventHandle(hEvent);
    if (FAILED(hr)) {
      std::cerr << "SetEventHandle failed: 0x" << std::hex << hr << std::dec << "\n";
      // not fatal; we'll continue with polling fallback
    }
  }
  // Get IAudioCaptureClient
  hr = m_audioClient->GetService(IID_PPV_ARGS(&m_captureClient));
  if (FAILED(hr)) { std::cerr << "GetService IAudioCaptureClient failed\n"; CloseHandle(hEvent); CoTaskMemFree(pwfx); CoUninitialize(); return false; }

  captured_buffer.reserve(static_cast<size_t>(m_srcRate * m_durationSec) * 4);
  // Interpret buffer according to format
  if (bFloat) {
    m_f = detail::make_processor_float;
  }
  else if (m_srcBits == 16) {
    // integer PCM 16 bit
    m_f = detail::make_processor_int<int16_t>;
  }
  else if (m_srcBits == 32) {
    m_f = detail::make_processor_int<int32_t>;
  }
  else {
    std::cerr << "Unsupported integer bit depth: " << m_srcBits << "\n";
    // release and abort
    m_stopFlag = true;
  }

  CoTaskMemFree(pwfx);
  return true;
};


bool AudioCapture::capture_loopback_buffer() {
  const size_t wanted_frames = static_cast<size_t>(std::ceil(m_srcRate * m_durationSec));
  bool done = false;
  uint32_t frames_captured = 0;

  while (!done) {
    // wait for event (if available) or poll
    DWORD waitMs = 500;
    DWORD waitRes = WAIT_OBJECT_0;
    if (hEvent) {
      waitRes = WaitForSingleObject(hEvent, waitMs);
      if (waitRes == WAIT_TIMEOUT) {
        // nothing arrived within timeout - continue loop to try again
        // but check if we already collected enough
        if (frames_captured >= wanted_frames) break;
        continue;
      }
    }
    else {
      Sleep(5);
    }
    if (m_stopFlag.load(std::memory_order_acquire) == true) {
      done = true;
      break;
    };

    UINT32 packetFrames = 0;
    HRESULT hr2 = m_captureClient->GetNextPacketSize(&packetFrames);
    // Read all available packets
    if (FAILED(hr2)) {
      std::cerr << "GetNextPacketSize failed\n";
      break;
    }
    if (!bFloat && m_srcBits != 16 && m_srcBits != 32)
    {
      std::cerr << "Unsupported integer bit depth: " << m_srcBits << "\n";
      done = true;
      break;
    }
    while (packetFrames > 0) {
      BYTE* pData = nullptr;
      UINT32 numFrames = 0;
      DWORD flags = 0;
      hr2 = m_captureClient->GetBuffer(&pData, &numFrames, &flags, nullptr, nullptr);
      if (FAILED(hr2)) {
        std::cerr << "GetBuffer failed\n";
        done = true;
        break;
      }
      if (flags & AUDCLNT_BUFFERFLAGS_SILENT) {
        // silent: push zeros
        for (UINT32 f = 0; f < numFrames; ++f) {
          captured_buffer.push(0.0f);
        }
      }
      else {
        m_f(m_srcCh, numFrames, pData, captured_buffer);
      }
      frames_captured += numFrames;
      // Release buffer with the exact numFrames
      hr2 = m_captureClient->ReleaseBuffer(numFrames);
      if (FAILED(hr2)) {
        std::cerr << "ReleaseBuffer failed\n";
        done = true;
        break;
      }

      // check length
      if (frames_captured >= wanted_frames) { done = true; break; }

      // get next packet size
      hr2 = m_captureClient->GetNextPacketSize(&packetFrames);
      if (FAILED(hr2)) { std::cerr << "GetNextPacketSize failed\n"; done = true; break; }
    } // while packetFrames
  } // while !done
  // stop & cleanup
  return true;
}

void AudioCapture::stop() {
  m_stopFlag.store(true, std::memory_order_release);
}
void AudioCapture::start_capture() {
  m_workerThread = std::thread([this]() {

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
    HRESULT hr = m_audioClient->Start();
    if (FAILED(hr)) {
      std::cerr << "audioClient Start failed\n"; CloseHandle(hEvent); CoUninitialize(); return false;
    }
    while (!m_stopFlag) {
      if (!capture_loopback_buffer()) {
        std::cerr << "capture_loopback_buffer failed\n";
        break;
      }
    }
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
  });
}
void AudioCapture::get_captured_buffer(containers::ring_buffer<float>& out) const 
  noexcept(noexcept(std::declval<containers::ring_buffer<float>&>().get_interval(std::declval<size_t>(), std::declval<size_t>(), std::declval<containers::ring_buffer<float>&>())))
{
  size_t frameSize = m_srcRate * m_durationSec;
  size_t head = captured_buffer.get_head();
  size_t idx = 0;
  if (head/frameSize != 0)
  {
    size_t idx = (frameSize * (head / (frameSize)-frameSize));// start of last full duration
  }
  captured_buffer.get_interval(idx, frameSize, out);
}

void AudioCapture::shutdown() {
  if (m_stopFlag.load(std::memory_order_acquire) == false)
  {
    stop();
    wait_for_completion();
    if (m_audioClient) {
      m_audioClient->Stop();
    }
    CoUninitialize();
    captured_buffer.~ring_buffer();
  }
}

void AudioCapture::clear_buffer() noexcept {
  captured_buffer.clear();
}
void AudioCapture::wait_for_completion() {
  if (m_workerThread.joinable()) {
    m_workerThread.join();
  }
}