
---

## Note

This project is intentionally **simple, unfinished, and honest** about its state.

If you are looking for a drop-in solution, this is not it.
If you are interested in building a **cross-platform real-time transcription engine**, this is the starting point.

---

# Real-Time Audio Transcription Engine

*(WASAPI / Whisper, early-stage)*

Early-stage, cross-platform **speech-to-text engine** based on **Whisper**, designed for real-time audio transcription.

The current implementation is a **Windows MVP** using WASAPI loopback capture.
Cross-platform support (Linux / macOS) is a **core design goal**, not yet fully implemented.

---

## Project Status

⚠️ **Early Development Stage**

This project is at an **initial MVP / prototype phase**.

* APIs are unstable
* Architecture is expected to change
* Performance and latency are not finalized
* Many features are incomplete or missing

The current code exists to **prove feasibility**, not to represent a finished system.

---

## Supported Platforms

### Current

* **Windows** (WASAPI loopback)

### Planned

* **Linux** (PulseAudio / PipeWire)
* **macOS** (Core Audio)
* Platform-independent audio abstraction layer

The long-term goal is a **single cross-platform audio + ASR pipeline** with platform-specific backends.

---

## Languages & Technologies

* **C++17**
* **Whisper (whisper.cpp)**
* Platform audio APIs:

  * WASAPI (Windows)
  * PulseAudio / PipeWire (planned)
  * Core Audio (planned)

The project is written in **C++** to allow:

* low-level audio control
* high performance
* native cross-platform builds

---

## Features (Current MVP)

* System audio capture (Windows loopback)
* Real-time audio streaming
* Downmix to mono
* On-the-fly resampling to 16 kHz
* Continuous Whisper inference
* Console text output
* Optional GPU acceleration

---

## What This Project Is

* A **foundation** for a cross-platform transcription engine
* A research and prototyping codebase
* A base for further architectural work
* A learning and experimentation project

## What This Project Is NOT

* Production-ready software
* A finished streaming ASR solution
* A polished user-facing application
* Stable or API-safe (yet)

---

## Architecture Overview

```
[ Audio Backend (per OS) ]
          ↓
[ Platform Abstraction Layer ]
          ↓
[ Audio Processing Pipeline ]
          ↓
[ Whisper Inference Engine ]
          ↓
[ Output (Console / API) ]
```

Current implementation covers only the **Windows backend**.

---

## Whisper Model

Whisper models must be downloaded manually.

Example:

```cpp
ggml-large-v3-turbo.bin
```

Model path is currently hardcoded and will be abstracted later.

---

## Configuration (Temporary)

Key parameters are defined directly in code:

* audio chunk size
* context length
* overlap
* thread count
* language

A proper configuration system is planned.

---

## Known Limitations

* Windows-only audio backend (for now)
* Naive resampling
* No real VAD
* No timestamps
* No device selection
* No clean shutdown handling
* No stable public API

All of this is expected at this stage.

---

## Roadmap (High-Level)

* Audio backend abstraction
* Linux & macOS support
* Proper resampling
* Streaming-friendly buffering
* Voice Activity Detection
* Timestamped output
* Modular inference interface
* Public API definition

---

## License

MIT License(see LICENSE.txt)

---