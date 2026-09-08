#include "voc.h"

#include <vector>
#include <stdexcept>

#include "utils.h"

namespace voc {

bool isVoc(const std::vector<uint8_t>& b)
{
    static const std::string kMagic = "Creative Voice File";
    return b.size() >= kMagic.size() && std::equal(kMagic.begin(), kMagic.end(), b.begin());
}

inline uint32_t rateFromTimeConstant(uint32_t tc, uint16_t channels, bool wide)
{
    if (wide)
    {
        const uint32_t d = channels * (65536u - tc);
        if (d == 0)
            throw std::runtime_error("bad VOC time constant");
        return 256000000u / d;
    }
    if (tc >= 256)
        throw std::runtime_error("bad VOC time constant");
    return 1000000u / (256u - tc);
}

inline uint16_t bitsForCodec(uint8_t codec)
{
    switch (codec) {
        case 0: return 8; // unsigned 8-bit PCM
        case 4: return 16; // signed 16-bit PCM
        case 1:
        case 2:
        case 3: throw std::runtime_error("VOC uses Creative ADPCM (codec " + std::to_string(codec) + "), not supported");
        case 6:
        case 7: throw std::runtime_error("VOC uses A-law/mu-law, not supported");
        default: throw std::runtime_error("unknown VOC codec " + std::to_string(codec));
    }
}

Audio decode(const std::vector<uint8_t>& b)
{
    if (!isVoc(b))
        throw std::runtime_error("not a Creative Voice File");

    const uint16_t dataOffset = Utils::readU16(b, 0x14);
    const uint16_t version = Utils::readU16(b, 0x16);
    const uint16_t checksum = Utils::readU16(b, 0x18);
    if (checksum != uint16_t(~version + 0x1234))
        throw std::runtime_error("VOC header checksum mismatch");
    if (dataOffset < 26 || dataOffset > b.size())
        throw std::runtime_error("bad VOC data offset");

    Audio out;
    bool haveFormat = false;

    // Set by a block 0x08, which describes the block 0x01 that follows it.
    bool pendingExtended = false;
    uint32_t pendingRate = 0;
    uint8_t pendingChannels = 1;
    uint16_t pendingBits = 8;

    auto adoptFormat = [&](uint32_t rate, uint8_t channels, uint16_t bits)
    {
        if (!haveFormat)
        {
            out.sampleRate = rate;
            out.channels = channels;
            out.bitsPerSample = bits;
            haveFormat = true;
        }
        else if (out.sampleRate != rate || out.channels != channels || out.bitsPerSample != bits)
        {
            // Concatenating blocks of differing formats would need resampling.
            throw std::runtime_error("VOC mixes multiple audio formats");
        }
    };

    size_t p = dataOffset;
    while (p < b.size()) {
        const uint8_t type = b[p];
        if (type == 0x00)
            break; // terminator
        if (p + 4 > b.size())
            throw std::runtime_error("truncated VOC block header");
        const size_t size = size_t(b[p + 1]) | (size_t(b[p + 2]) << 8) | (size_t(b[p + 3]) << 16);
        const size_t body = p + 4;
        if (body + size > b.size())
            throw std::runtime_error("truncated VOC block body");

        switch (type)
        {
            case 0x01: // sound data
            {
                if (size < 2) throw std::runtime_error("short VOC sound block");
                if (pendingExtended) {
                    adoptFormat(pendingRate, pendingChannels, pendingBits);
                    pendingExtended = false;
                } else {
                    adoptFormat(rateFromTimeConstant(b[body], 1, false), 1, bitsForCodec(b[body + 1]));
                }
                out.samples.insert(out.samples.end(), b.begin() + body + 2, b.begin() + body + size);
                break;
            }
            case 0x02: // sound continuation: reuses the previous format
            {
                if (!haveFormat) throw std::runtime_error("VOC continuation before any sound block");
                out.samples.insert(out.samples.end(), b.begin() + body, b.begin() + body + size);
                break;
            }
            case 0x03: // silence
            {
                if (size < 3) throw std::runtime_error("short VOC silence block");
                const uint32_t count = uint32_t(Utils::readU16(b, body)) + 1;
                adoptFormat(rateFromTimeConstant(b[body + 2], 1, false), 1, 8);
                out.samples.insert(out.samples.end(),  size_t(count) * out.channels * (out.bitsPerSample / 8), out.getSilenceByte());
                break;
            }
            case 0x08: // extended format, describes the next 0x01 block
            {
                if (size < 4)
                    throw std::runtime_error("short VOC extended block");
                pendingChannels = uint8_t(b[body + 3] ? 2 : 1);
                pendingRate = rateFromTimeConstant(Utils::readU16(b, body), pendingChannels, true);
                pendingBits = bitsForCodec(b[body + 2]);
                pendingExtended = true;
                break;
            }
            case 0x09: // sound data, VOC 1.20+
            {
                if (size < 12)
                    throw std::runtime_error("short VOC 1.20 sound block");
                const uint32_t rate = Utils::readU32(b, body);
                const uint16_t bits = b[body + 4];
                const uint16_t channels = b[body + 5];
                if (bits != 8 && bits != 16)
                    throw std::runtime_error("unsupported VOC bit depth " + std::to_string(bits));
                adoptFormat(rate, channels ? channels : 1, bits);
                out.samples.insert(out.samples.end(), b.begin() + body + 12, b.begin() + body + size);
                break;
            }
            case 0x04: // marker
            case 0x05: // ASCII text
            case 0x06: // repeat start
            case 0x07: // repeat end
                break; // playback metadata, nothing to render
            default:
                break; // unknown block: skip using its declared size
        }

        p = body + size;
    }

    if (!haveFormat || out.samples.empty())
        throw std::runtime_error("VOC contains no audio");

    return out;
}

} // namespace voc
