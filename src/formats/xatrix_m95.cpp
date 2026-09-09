#include "xatrix.h"

#include <vector>
#include <filesystem>

#include "utils.h"
#include "voc.h"
#include "wave.h"

namespace m95 {

constexpr size_t kBlockSize = 2048;
constexpr size_t kHeaderDwords = 23;
constexpr size_t kFrameSlots = 16;
constexpr size_t kAudioSlot = 16;
constexpr size_t kPaletteSlot = 20;
constexpr size_t kPaletteBytes = 768;

struct ChunkInfo
{
    uint32_t startBlock = 0;
    uint32_t blockCount = 0;
    uint32_t frameCount = 0;
    uint32_t audioOffset = 0; 
    uint32_t paletteOffset = 0;
    std::vector<std::pair<uint32_t, uint32_t>> frames;

    size_t offset() const { return size_t(startBlock) * kBlockSize; }
    size_t size() const { return size_t(blockCount) * kBlockSize; }
};

inline std::vector<ChunkInfo> parse(const std::vector<uint8_t>& d)
{
    if (d.size() < kBlockSize)
        throw std::runtime_error("file too small to be a .M95");

    // Detect empty/placeholder movies
    if (Utils::readU16(d, 0) == 0 && d[2] == 0)
        return {};

    const size_t dirEnd = size_t(Utils::readU16(d, 0)) * kBlockSize;
    if (dirEnd == 0 || dirEnd > d.size())
        throw std::runtime_error("not a .M95 file");

    std::vector<ChunkInfo> chunks;
    for (size_t p = 0; p + 4 <= dirEnd; p += 4)
    {
        const uint16_t start = Utils::readU16(d, p);
        const uint8_t blocks = d[p + 2];
        if (start == 0 && blocks == 0)
            break; // terminator

        ChunkInfo c;
        c.startBlock = start;
        c.blockCount = blocks;
        c.frameCount = d[p + 3];
        chunks.push_back(std::move(c));
    }

    if (chunks.empty())
        throw std::runtime_error("not a .M95 file (empty directory)");

    for (size_t ci = 0; ci < chunks.size(); ++ci)
    {
        ChunkInfo& c = chunks[ci];
        const size_t base = c.offset();
        const size_t end = base + c.size();
        if (end > d.size() || c.frameCount > kFrameSlots)
            throw std::runtime_error("chunk " + std::to_string(ci) + " is out of range");

        std::vector<uint32_t> hdr(kHeaderDwords);
        for (size_t k = 0; k < kHeaderDwords; ++k)
            hdr[k] = Utils::readU32(d, base + k * 4);

        if (hdr[kAudioSlot]) c.audioOffset = uint32_t(base) + hdr[kAudioSlot];
        if (hdr[kPaletteSlot]) c.paletteOffset = uint32_t(base) + hdr[kPaletteSlot];

        size_t tail = c.size();
        if (hdr[kAudioSlot]) tail = std::min<size_t>(tail, hdr[kAudioSlot]);
        if (hdr[kPaletteSlot]) tail = std::min<size_t>(tail, hdr[kPaletteSlot]);

        for (size_t j = 0; j < c.frameCount; ++j) {
            const size_t from = hdr[j];
            const size_t to = (j + 1 < c.frameCount) ? hdr[j + 1] : tail;
            if (from >= to || base + to > end)
                throw std::runtime_error("frame " + std::to_string(j) + " of chunk " +
                                         std::to_string(ci) + " is out of range");
            c.frames.emplace_back(uint32_t(base + from), uint32_t(to - from));
        }
    }
    return chunks;
}

inline voc::Audio readRiff(const std::vector<uint8_t>& d, size_t p, size_t limit)
{
    limit = std::min(limit, d.size());
    if (p + 12 > limit || d[p] != 'R' || d[p + 1] != 'I' || d[p + 2] != 'F' || d[p + 3] != 'F')
        throw std::runtime_error("expected a RIFF header");

    const size_t end = std::min(limit, p + 8 + size_t(Utils::readU32(d, p + 4)));

    voc::Audio a;
    bool haveFmt = false, haveData = false;
    size_t q = p + 12;
    while (q + 8 <= end)
    {
        const uint32_t id = Utils::readU32(d, q);
        const size_t size = Utils::readU32(d, q + 4);
        if (id == 0)
            break; // filler
        if (q + 8 + size > end)
            break; // stale bytes past the real content

        if (id == 0x20746D66u) // "fmt "
        {
            if (size < 16)
                throw std::runtime_error("short fmt chunk");
            if (Utils::readU16(d, q + 8) != 1)
                throw std::runtime_error("RIFF audio is not PCM");

            a.channels = (uint8_t)Utils::readU16(d, q + 10);
            a.sampleRate = Utils::readU32(d, q + 12);
            a.bitsPerSample = Utils::readU16(d, q + 22);
            haveFmt = true;
        }
        else if (id == 0x61746164u) // "data"
        {
            a.samples.assign(d.begin() + q + 8, d.begin() + q + 8 + size);
            haveData = true;
        }
        q += 8 + size + (size & 1); // sub-chunks are word aligned
    }
    if (!haveFmt || !haveData)
        throw std::runtime_error("RIFF is missing fmt or data");

    return a;
}

inline bool getAudioTrack(const std::vector<uint8_t>& d, const std::vector<ChunkInfo>& chunks, voc::Audio& out)
{
    bool first = true;
    for (size_t ci = 0; ci < chunks.size(); ++ci)
    {
        const ChunkInfo& c = chunks[ci];
        if (!c.audioOffset)
            continue;

        const voc::Audio a = readRiff(d, c.audioOffset, c.offset() + c.size());
        if (first)
        {
            out = a;
            first = false;
        }
        else
        {
            if (a.sampleRate != out.sampleRate || a.channels != out.channels || a.bitsPerSample != out.bitsPerSample)
                throw std::runtime_error("audio format changes mid-movie");
            out.samples.insert(out.samples.end(), a.samples.begin(), a.samples.end());
        }
    }
    return !first;
}

}  // namespace m95


namespace xatrix {

void m95_to_wav(const std::string& in_path, const std::string& in_outFilePath)
{
    std::ifstream file(in_path, std::ios::binary);
    if (!file.is_open())
        return;

    const auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto stem = std::filesystem::path(in_path).stem().string();

    auto outDir = std::filesystem::path(in_outFilePath);

    const auto chunks = m95::parse(data);

    size_t frameCount = 0;
    for (const auto& c : chunks)
        frameCount += c.frames.size();

    // one wav (glued together) for the whole movie
    voc::Audio audio;
    if (m95::getAudioTrack(data, chunks, audio))
    {
        std::filesystem::create_directories(outDir);

        std::string outPath = (outDir / (stem + ".wav")).string();
        Wave::write(outPath, (char*)audio.samples.data(), (uint32_t)audio.samples.size(), audio.channels, audio.sampleRate, audio.bitsPerSample);
    }
}

} // namespace xatrix
