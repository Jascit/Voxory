#pragma once
#include <atomic>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <string>
#include <windows.h>
#include <iostream>
#include <platform/VirtualAlloc.h>

// ---------- Lock-free SPSC ring (chunk-aligned) ----------
class ChunkRing {
public:
  // capacity_slots must be >= 2; chunk_samples > 0
  ChunkRing(size_t capacity_slots, size_t chunk_samples)
    : slots(capacity_slots), chunk_sz(chunk_samples),
    total_samples(capacity_slots* chunk_samples),
    buffer(nullptr), is_buffer_locked(false)
  {
    size_t bytes = total_samples * sizeof(float);

    // виділяємо сторінково і блокуємо мапінг/свап
    buffer = static_cast<float*>(VirtualAlloc(
      nullptr, bytes,
      MEM_RESERVE | MEM_COMMIT,
      PAGE_READWRITE
    ));

    if (!buffer) {
      throw std::bad_alloc();
    }

    // блокуємо сторінки в RAM, щоб система не мала права їх підкачувати
    if (!TryVirtualLock(buffer, bytes, is_buffer_locked)) {
      DWORD err = GetLastError();
      VirtualFree(buffer, 0, MEM_RELEASE);
      throw std::runtime_error("VirtualLock failed with code " + std::to_string(err));
    }

    std::fill_n(buffer, total_samples, 0.0f);

    write_index.store(0, std::memory_order_relaxed);
    read_index.store(0, std::memory_order_relaxed);
  }

  ~ChunkRing() {
    if (buffer) {
      // розблоковуємо перед звільненням
      if (is_buffer_locked)
      {
        VirtualUnlock(buffer, total_samples * sizeof(float));
      }
      VirtualFree(buffer, 0, MEM_RELEASE);
      buffer = nullptr;
    }
  }

  // Producer: write 'count' samples (count must == chunk_sz). returns false if stop flag set.
  // We copy into the slot; (we could try to avoid even this memcpy if capture could write directly to slot)
  bool push_chunk(const float* src, std::atomic<bool>& stop_flag) {
    if (stop_flag.load(std::memory_order_relaxed)) return false;

    // reserve a slot by incrementing write_index (we'll write into slot idx)
    uint64_t w = write_index.load(std::memory_order_relaxed);
    uint64_t r = read_index.load(std::memory_order_acquire); // ensure we see latest read

    // detect overwrite: if write - read >= slots, advance read to keep distance < slots
    if ((w - r) >= slots) {
      // Drop oldest
      uint64_t new_r = w - (slots - 1);
      read_index.store(new_r, std::memory_order_release);
      r = new_r;
    }

    size_t slot = (size_t)(w % slots);
    float* dst = buffer + slot * chunk_sz;
    // copy chunk_sz floats into dst
    std::memcpy(dst, src, chunk_sz * sizeof(float));
    // publish the write by incrementing write_index (release so consumer sees data)
    write_index.store(w + 1, std::memory_order_release);
    return true;
  }

  // Consumer: if there is a chunk available, return pointer to chunk and advance read index.
  // Returns nullptr if none available.
  // The pointer is valid until that slot is overwritten by producer (i.e. until write_index advances enough).
  const float* try_pop_chunk(size_t& out_count) {
    uint64_t r = read_index.load(std::memory_order_relaxed);
    uint64_t w = write_index.load(std::memory_order_acquire);
    if (r >= w) {
      out_count = 0;
      return nullptr; // no data
    }
    size_t slot = (size_t)(r % slots);
    const float* ptr = buffer + slot * chunk_sz;
    out_count = chunk_sz;
    // advance read index (release): consumer finished this chunk and claims it
    read_index.store(r + 1, std::memory_order_release);
    return ptr;
  }

  size_t get_chunk_samples() const { return chunk_sz; }
  size_t get_slots() const { return slots; }

private:
  const size_t slots;
  const size_t chunk_sz;
  const size_t total_samples;
  float* buffer;
  std::atomic<uint64_t> write_index; // monotonic increasing
  std::atomic<uint64_t> read_index;  // monotonic increasing
  bool is_buffer_locked;
};
