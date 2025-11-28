#pragma once
#include <filesystem>
#include <vector>
#include <map>

namespace interfaces {
  class IAudioDecoder {
  public:
    virtual ~IAudioDecoder() = default;

    virtual std::vector<float> decode(const std::filesystem::path& filename) = 0;
    virtual void encode(const std::filesystem::path& filename,
      const std::vector<float>& data,
      int sampleRate,
      int channels) = 0;
    virtual std::map<std::string, double> getInfo(const std::string& filename) = 0;
  };
}