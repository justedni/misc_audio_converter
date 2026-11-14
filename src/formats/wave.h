#pragma once

#include "common.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Wave {

constexpr static char kRIFF[4] = { 'R', 'I', 'F', 'F' };

PACK(struct WavHeader
{
    // RIFF header
    char tagRIFF[4];
    int wavSize;
    char tagWave[4];

    // Fmt header
    char tagFmt[4];
    int fmtChunkSize;
    short audioFormat;
    short numChannels;
    int sampleRate;
    int byteRate;
    short sampleAlignment;
    short bitDepth;

    // Data
    char tagData[4];
    int dataChunkSize;
});
static_assert(sizeof(WavHeader) == 44, "");

struct WavData
{
    uint16_t audioFormat = 0;
    uint16_t numChannels = 0;
    uint32_t sampleRate = 0;
    uint16_t bitDepth = 0;
    std::vector<char> samples;
};

bool read(const std::string& path, WavData& out);

void write(const std::string& path, char* pData, uint32_t dataSize, uint8_t numChannels, uint32_t sampleRate, uint32_t bitSize);

} // namespace Wave
