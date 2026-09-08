#include "xatrix.h"

#include <vector>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <memory>
#include <fstream>
#include <assert.h>

#include "voc.h"
#include "wave.h"

#include "utils.h"

namespace z94 {


struct DirEntry
{
    uint32_t offset = 0;
    uint32_t flags = 0;
    uint32_t compressedSize = 0;
    uint32_t uncompressedSize = 0;
};

inline std::vector<DirEntry> readDirectory(const std::vector<uint8_t>& data)
{
    const uint32_t dirSize = Utils::readU32(data, 0);
    if (dirSize < 16 || dirSize % 16 != 0 || dirSize > data.size())
    {
        assert(false);
        return {};
    }

    std::vector<DirEntry> out;
    for (uint32_t p = 0; p < dirSize; p += 16)
    {
        DirEntry entry;
        entry.offset = Utils::readU32(data, p);
        entry.flags = Utils::readU32(data, p + 4);
        entry.compressedSize = Utils::readU32(data, p + 8);
        entry.uncompressedSize = Utils::readU32(data, p + 12);
        out.push_back(std::move(entry));
    }
    return out;
}

inline std::vector<uint8_t> readCodeLengths(const std::vector<uint8_t>& buf, size_t& p, size_t numSymbols)
{
    if (p >= buf.size())
    {
        assert(false); // Truncated table header
        return {};
    }

    const size_t packedCount = size_t(buf[p++]) + 1;

    std::vector<uint8_t> lengths;
    lengths.reserve(numSymbols);
    for (size_t i = 0; i < packedCount; ++i)
    {
        if (p >= buf.size())
        {
            assert(false); // Truncated table header
            return {};
        }

        const uint8_t b = buf[p++];
        const size_t run = size_t(b >> 4) + 1;
        const uint8_t len = uint8_t(b & 0x0F) + 1;
        lengths.insert(lengths.end(), run, len);
    }

    assert(lengths.size() == numSymbols);
    return lengths;
}

class BitReader {
public:
    BitReader(const uint8_t* data, size_t size, size_t pos) : data_(data), size_(size)
    {
        if (pos + 2 > size)
        {
            assert(false); // Truncated bitstream
            return;
        }

        dx_ = uint32_t(data[pos]) | (uint32_t(data[pos + 1]) << 8);
        pos_ = pos + 2;
        ch_ = 8;
    }

    uint32_t take(int n)
    {
        const uint32_t v = dx_ & ((1u << n) - 1u);
        dx_ >>= n;
        ch_ -= n;
        if (ch_ < 0)
        {
            ch_ += 8;
            const uint32_t next = pos_ < size_ ? data_[pos_] : 0u;
            ++pos_;
            dx_ = (dx_ | (next << ch_)) & 0xFFFF;
        }
        return v;
    }

private:
    const uint8_t* data_ = nullptr;
    size_t size_ = 0, pos_ = 0;
    uint32_t dx_ = 0;
    int ch_ = 0;
};


class Huffman
{
public:
    static constexpr int kMaxLength = 16;

    explicit Huffman(const std::vector<uint8_t>& lengths)
    {
        std::vector<int> order(lengths.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](int a, int b){ return lengths[a] != lengths[b] ? lengths[a] < lengths[b] : a < b; });

        uint32_t cx = 0, dx = 0;
        int prevLength = 0;
        for (auto it = order.rbegin(); it != order.rend(); ++it)
        {
            const int sym = *it;
            const int len = lengths[sym];
            assert(len >= 1 && len <= kMaxLength);

            cx = (cx + dx) & 0xFFFF;
            if (prevLength != len)
            {
                prevLength = len;
                dx = 1u << (16 - len);
            }
            const uint32_t code = cx >> (16 - len);

            if (symbols_[len].empty())
                minCode_[len] = code;
            symbols_[len].push_back(uint16_t(sym));
        }
    }

    inline int decode(BitReader& bits) const
    {
        uint32_t code = 0;
        for (int len = 1; len <= kMaxLength; ++len)
        {
            code = (code << 1) | bits.take(1);
            const auto& syms = symbols_[len];
            if (!syms.empty() && code >= minCode_[len] && code - minCode_[len] < syms.size())
                return syms[code - minCode_[len]];
        }

        assert(false); // invalid Huffman code
        return 0;
    }

private:
    uint32_t minCode_[kMaxLength + 1] = {};
    std::vector<uint16_t> symbols_[kMaxLength + 1];
};

std::vector<uint8_t> decompress(const uint8_t* data, size_t size, uint32_t uncompressedSize, uint32_t flags)
{
    if ((flags & 1) == 0) // no compression
    {
        std::vector<uint8_t>  stored(data, data + size);
        stored.resize(uncompressedSize);
        return stored;
    }

    const std::vector<uint8_t> buf(data, data + size);
    size_t p = 0;

    int lengthBase = 2;
    int offsetBits = 6;
    std::unique_ptr<Huffman> literalTable;

    if (flags & 2) offsetBits = 7;
    if (flags & 4)
    {
        lengthBase = 3;
        literalTable = std::make_unique<Huffman>(readCodeLengths(buf, p, 0x100));
    }

    const Huffman lengthTable(readCodeLengths(buf, p, 0x40));
    const Huffman offsetTable(readCodeLengths(buf, p, 0x40));

    BitReader bits(data, size, p);
    std::vector<uint8_t> out;
    out.reserve(uncompressedSize);

    while (out.size() < uncompressedSize)
    {
        // 1 -> literal
        if (bits.take(1))
        {
            out.push_back(uint8_t(literalTable ? literalTable->decode(bits) : bits.take(8)));
            continue;
        }

        // 0 -> match
        const uint32_t low = bits.take(offsetBits);
        const uint32_t offset = ((uint32_t(offsetTable.decode(bits)) << offsetBits) | low) + 1;

        int64_t length = int64_t(lengthTable.decode(bits)) + lengthBase;
        if (length == lengthBase + 0x3F) // escape: 8 extra raw bits
            length += bits.take(8);

        if (offset > out.size())
        {
            const uint16_t written = uint16_t(out.size());
            uint16_t ax = uint16_t(uint16_t(uncompressedSize) - written);
            uint16_t cx = uint16_t(uint16_t(offset) - written);
            if (ax <= cx) { // CMP AX,CX / JA - unsigned
                cx = ax;
                length = ax;
            }
            length -= cx;
            out.insert(out.end(), size_t(cx), 0);
        }

        const size_t src = out.size() - offset;
        for (int64_t i = 0; i < length; ++i)
            out.push_back(out[src + size_t(i)]);
    }

    out.resize(uncompressedSize);
    return std::move(out);
}

inline bool splitOffsetTable(const std::vector<uint8_t>& blob, std::vector<std::vector<uint8_t>>& out_parts)
{
    if (blob.size() < 8)
        return false;

    const uint32_t tableSize = Utils::readU32(blob, 0);
    if (tableSize % 4 != 0 || tableSize < 8 || tableSize > blob.size())
        return false;

    std::vector<uint32_t> offsets(tableSize / 4);
    for (size_t i = 0; i < offsets.size(); ++i) offsets[i] = Utils::readU32(blob, i * 4);

    if (offsets.back() != blob.size())
        return false;
    if (!std::is_sorted(offsets.begin(), offsets.end()))
        return false;

    for (size_t i = 0; i + 1 < offsets.size(); ++i)
        out_parts.emplace_back(blob.begin() + offsets[i], blob.begin() + offsets[i + 1]);

    return true;
}

} // namespace z94

namespace xatrix {

void z94_to_wav(const std::string& in_path, const std::string& in_outFilePath)
{
    std::ifstream file(in_path, std::ios::binary);
    if (!file.is_open())
        return;

    const auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto stem = std::filesystem::path(in_path).stem().string();

    auto outDir = std::filesystem::path(in_outFilePath) / stem;

    const auto dir = z94::readDirectory(data);

    std::vector<std::vector<uint8_t>> vocFiles;
    for (size_t i = 0; i < dir.size(); ++i)
    {
        const auto& e = dir[i];
        if (size_t(e.offset) + e.compressedSize > data.size())
        {
            assert(false); // Entry runs past end of file
            return;
        }

        auto blob = z94::decompress(data.data() + e.offset, e.compressedSize, e.uncompressedSize, e.flags);

        std::vector<std::vector<uint8_t>> parts;
        if (z94::splitOffsetTable(blob, parts))
        {
            for (auto& part : parts)
            {
                if (voc::isVoc(part))
                    vocFiles.push_back(std::move(part));
            }
        }
        else
        {
            if (voc::isVoc(blob))
                vocFiles.push_back(std::move(blob));
        }
    }

    if (!vocFiles.empty())
    {
        std::filesystem::create_directories(outDir);
    }

    int count = 0;
    for (auto& blob : vocFiles)
    {
        assert(voc::isVoc(blob));

        const voc::Audio audio = voc::decode(blob);

        auto filename = Utils::str_format("%02d.wav", count);
        std::string outPath = (outDir / filename).string();

        Wave::write(outPath, (char*)audio.samples.data(), (uint32_t)audio.samples.size(), audio.channels, audio.sampleRate, audio.bitsPerSample);
        count++;
    }
}

} // namespace xatrix
