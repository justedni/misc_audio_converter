#pragma once

#include <string>
#include <vector>

namespace voc {

struct Audio
{
    uint32_t sampleRate = 0;
    uint8_t channels = 1;
    uint16_t bitsPerSample = 8;
    std::vector<uint8_t> samples;

    uint8_t getSilenceByte() const { return bitsPerSample == 8 ? 0x80 : 0x00; }
};

bool isVoc(const std::vector<uint8_t>& b);
Audio decode(const std::vector<uint8_t>& b);

} // namespace voc
