#include "xatrix.h"

#include <vector>
#include <filesystem>

#include "utils.h"
#include "voc.h"
#include "wave.h"

namespace c93 {

constexpr size_t kBlockSize = 2048;
constexpr size_t kFrameTableSize = 128;

struct ChunkInfo
{
    uint32_t startBlock = 0;
    uint32_t blockCount = 0;
    uint32_t frameCount = 0;
    size_t offset() const { return size_t(startBlock) * kBlockSize; }
    size_t size() const { return size_t(blockCount) * kBlockSize; }
};

struct Frame {
    size_t chunk = 0;
    size_t index = 0;
    size_t videoOffset = 0;
    size_t videoSize = 0;
    size_t paletteOffset = 0;
    size_t paletteSize = 0;
    size_t audioOffset = 0;
    size_t audioSize = 0;
};

struct Movie {
    std::vector<ChunkInfo> chunks;
    std::vector<Frame> frames;
};

inline std::vector<ChunkInfo> readDirectory(const std::vector<uint8_t>& d)
{
    std::vector<ChunkInfo> chunks;
    for (size_t p = 0; p + 4 <= kBlockSize; p += 4)
    {
        const uint16_t start = Utils::readU16(d, p);
        const uint8_t blocks = d[p + 2];
        if (start == 0 && blocks == 0)
            break; // terminator
        chunks.push_back({ start, blocks, d[p + 3] });
    }
    if (chunks.empty())
        throw std::runtime_error("not a .C93 file (empty directory)");
    return chunks;
}


inline Movie parse(const std::vector<uint8_t>& d)
{
    Movie m;
    m.chunks = readDirectory(d);

    for (size_t ci = 0; ci < m.chunks.size(); ++ci)
    {
        const ChunkInfo& c = m.chunks[ci];
        const size_t base = c.offset();
        const size_t end = base + c.size();
        if (end > d.size() || c.frameCount * 4 > kFrameTableSize)
            throw std::runtime_error("chunk " + std::to_string(ci) + " is out of range");

        for (size_t j = 0; j < c.frameCount; ++j)
        {
            size_t p = base + Utils::readU32(d, base + j * 4);
            if (p >= end)
                throw std::runtime_error("frame offset out of range");

            Frame f;
            f.chunk = ci;
            f.index = j;
            f.videoSize = Utils::readU16(d, p);
            f.videoOffset = p + 2;
            p = f.videoOffset + f.videoSize;
            f.paletteSize = Utils::readU16(d, p);
            f.paletteOffset = p + 2;
            p = f.paletteOffset + f.paletteSize;
            f.audioSize = Utils::readU16(d, p);
            f.audioOffset = p + 2;
            p = f.audioOffset + f.audioSize;
            if (p > end)
                throw std::runtime_error("frame " + std::to_string(j) + " of chunk " + std::to_string(ci) + " overruns its chunk");
            m.frames.push_back(std::move(f));
        }
    }
    return m;
}

inline bool getAudioTrack(const std::vector<uint8_t>& d, const Movie& m, voc::Audio& out_audio)
{
    bool first = true;
    for (const Frame& f : m.frames)
    {
        if (f.audioSize == 0)
            continue;

        const std::vector<uint8_t> slice(d.begin() + f.audioOffset, d.begin() + f.audioOffset + f.audioSize);
        const voc::Audio a = voc::decode(slice);
        if (first)
        {
            out_audio = a;
            first = false;
        }
        else
        {
            if (a.sampleRate != out_audio.sampleRate || a.channels != out_audio.channels || a.bitsPerSample != out_audio.bitsPerSample)
                throw std::runtime_error("audio format changes mid-movie");

            out_audio.samples.insert(out_audio.samples.end(), a.samples.begin(), a.samples.end());
        }
    }

    return (!first);
}

} // namespace c93

namespace xatrix {

bool isXatrixFile(EFileType type)
{
    return (type == EFileType::XatrixZ94 || type == EFileType::XatrixC93 || type == EFileType::XatrixC93);
}

void c93_to_wav(const std::string& in_path, const std::string& in_outFilePath)
{
    std::ifstream file(in_path, std::ios::binary);
    if (!file.is_open())
        return;

    const auto data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const auto stem = std::filesystem::path(in_path).stem().string();

    auto outDir = std::filesystem::path(in_outFilePath);

    const c93::Movie m = c93::parse(data);

    // one wav (glued together) for the whole movie
    voc::Audio audio;
    if (!c93::getAudioTrack(data, m, audio))
        return;

    std::filesystem::create_directories(outDir);

    std::string outPath = (outDir / (stem + ".wav")).string();
    Wave::write(outPath, (char*)audio.samples.data(), (uint32_t)audio.samples.size(), audio.channels, audio.sampleRate, audio.bitsPerSample);
}

} // namespace xatrix
