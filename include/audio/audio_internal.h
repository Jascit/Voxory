#pragma once
#include <iostream>
#include <vector>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <wrl/client.h>
#include <mmreg.h> // for WAVEFORMATEXTENSIBLE
#include <stdint.h>
#include <cmath>

#pragma comment(lib, "Ole32.lib")
#pragma comment(lib, "Avrt.lib")

using Microsoft::WRL::ComPtr;

// float -> int16 clamp
static inline int16_t f32_to_s16_clamped(float v) {
  if (v >= 1.0f) return INT16_MAX;
  if (v <= -1.0f) return INT16_MIN;
  return static_cast<int16_t>(std::lrintf(v * 32767.0f));
}

// Linear resampler (mono float)
static std::vector<float> resample_linear(const std::vector<float>& src, uint32_t src_rate, uint32_t dst_rate) {
  if (src_rate == dst_rate) return src;
  const double ratio = static_cast<double>(src_rate) / static_cast<double>(dst_rate);
  const size_t dst_len = static_cast<size_t>(std::ceil(src.size() / ratio));
  std::vector<float> dst(dst_len);
  for (size_t i = 0; i < dst_len; ++i) {
    double src_pos = i * ratio;
    size_t i0 = static_cast<size_t>(std::floor(src_pos));
    size_t i1 = i0 + 1;
    double frac = src_pos - i0;
    float s0 = (i0 < src.size()) ? src[i0] : 0.0f;
    float s1 = (i1 < src.size()) ? src[i1] : 0.0f;
    dst[i] = static_cast<float>(s0 * (1.0 - frac) + s1 * frac);
  }
  return dst;
}
