#include "wave.h"

#include "utils.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>

namespace Wave {

constexpr static char kWAVE[4] = { 'W', 'A', 'V', 'E' };
constexpr static char kfmt[4] = { 'f', 'm', 't', ' ' };
constexpr static char kdata[4] = { 'd', 'a', 't', 'a' };

bool read(const std::string& path, WavData& out)
{
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return false;

    const std::size_t fileSize =static_cast<std::size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize))
        return false;

    if (fileSize < 12 || memcmp(&buffer[0], kRIFF, 4) != 0 || memcmp(&buffer[8], kWAVE, 4) != 0)
        return false;

    bool bFoundFmt = false;
    bool bFoundData = false;

    std::size_t pos = 12;
    while (pos + 8 <= fileSize && !bFoundData)
    {
        const uint8_t* chunkId = &buffer[pos];
        const std::size_t chunkSize = Utils::readU32(buffer, pos + 4);
        const std::size_t chunkStart = pos + 8;
        const std::size_t available = fileSize - chunkStart;

        if (memcmp(chunkId, kfmt, 4) == 0)
        {
            if (chunkSize < 16 || chunkSize > available)
                return false;
 
            out.audioFormat = Utils::readU16(buffer, chunkStart);
            out.numChannels = Utils::readU16(buffer, chunkStart + 2);
            out.sampleRate = Utils::readU32(buffer, chunkStart + 4);
            out.bitDepth = Utils::readU16(buffer, chunkStart + 14);
            bFoundFmt = true;
        }
        else if (memcmp(chunkId, kdata, 4) == 0)
        {
            const std::size_t dataSize = std::min(chunkSize, available);
            out.samples.assign(buffer.begin() + chunkStart, buffer.begin() + chunkStart + dataSize);
            bFoundData = true;
        }

        pos = chunkStart + chunkSize + (chunkSize & 1);
    }

    if (!bFoundFmt || !bFoundData)
        return false;

    return true;
}

void write(const std::string& path, char* pData, uint32_t dataSize, uint8_t numChannels, uint32_t sampleRate, uint32_t bitSize)
{
    using namespace Utils;

    std::ofstream os(path, std::ofstream::binary);
    os.write(kRIFF, 4);
    writeInt(os, (uint32_t)(36 + dataSize));
    os.write(kWAVE, 4);

    os.write(kfmt, 4);
    writeInt(os, (uint32_t)16);
    writeInt(os, (uint16_t)1);
    writeInt(os, (uint16_t)numChannels);
    writeInt(os, (uint32_t)sampleRate);
    uint16_t blockAlign = (numChannels * bitSize / 8);
    writeInt(os, (uint32_t)(sampleRate * blockAlign));
    writeInt(os, (uint16_t)blockAlign);
    writeInt(os, (uint16_t)bitSize);

    os.write(kdata, 4);
    writeInt(os, (uint32_t)(dataSize));

    os.write((char*)pData, dataSize);
}

} // namespace Wave
