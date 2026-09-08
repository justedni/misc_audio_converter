#include "utils.h"

#include <assert.h>
#include <algorithm>

namespace Utils
{
    void peekChar(std::ifstream& stream, char* data, size_t size)
    {
        auto pos = stream.tellg();
        stream.read(data, size);
        stream.seekg(pos, std::ios::beg);
    }

    uint16_t swap16(uint16_t x)
    {
        uint16_t hi = (x & 0xff00) >> 8;
        uint16_t lo = (x & 0xff);
        return static_cast<uint16_t>(lo << 8) | hi;
    }
    int16_t swap16(int16_t x)
    {
        int16_t hi = (x & 0xff00) >> 8;
        int16_t lo = (x & 0xff);
        return static_cast<int16_t>(lo << 8) | hi;
    }

    uint32_t readU32(const std::vector<uint8_t>& b, size_t p)
    {
        assert(p + 4 <= b.size());
        return uint32_t(b[p]) | (uint32_t(b[p + 1]) << 8) | (uint32_t(b[p + 2]) << 16) | (uint32_t(b[p + 3]) << 24);
    }

    uint16_t readU16(const std::vector<uint8_t>& b, size_t p)
    {
        assert(p + 2 <= b.size());
        return uint16_t(b[p] | (b[p + 1] << 8));
    }

    std::string str_to_lower(const std::string& inputStr)
    {
        std::string retStr = inputStr;
        std::transform(retStr.begin(), retStr.end(), retStr.begin(),
            [](unsigned char c) { return std::tolower(c); });

        return retStr;
    }

} // namespace Utils
