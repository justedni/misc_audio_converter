#include "inti_icelib.h"

//-----------------------------------------------------------------------------
// icelib functions extracted (and enhanced) from the vgmstream repository
// https://github.com/vgmstream/vgmstream/blob/master/src/coding/libs/icelib.c
//-----------------------------------------------------------------------------
namespace icelib {

uint32_t get_u32be(const uint8_t* mem) {
    return ((uint32_t)mem[0] << 24) | ((uint32_t)mem[1] << 16) | ((uint32_t)mem[2] << 8) | (uint32_t)mem[3];
}

uint32_t get_u32le(const uint8_t* mem) {
    return ((uint32_t)mem[3] << 24) | ((uint32_t)mem[2] << 16) | ((uint32_t)mem[1] << 8) | (uint32_t)mem[0];
}

uint16_t get_u16le(const uint8_t* mem) {
    return ((uint16_t)mem[1] << 8) | (uint16_t)mem[0];
}

uint8_t get_u8(const uint8_t* p) { return (uint8_t)p[0]; }

/* read main .bigrp header. Earlier games used standard Nintendo's BFGRP and BCGRP,
 * and this format seems kind of inspired by it, so presumably BIGRP = Binary Inti Group */
bool parse_bigrp_header(bigrp_header_t* hdr, const uint8_t* buf, int buf_size)
{
    if (buf_size < 0x0c)
        return false;

    /* read base header */
    hdr->head_size = get_u32le(buf + 0x00);
    hdr->entry_size = get_u32le(buf + 0x04);
    hdr->total_subsongs = get_u32le(buf + 0x08);

    if (hdr->head_size > buf_size)
        return false;

    if (hdr->head_size >= 0x10)
        hdr->dummy = get_u32le(buf + 0x0c);
    else
        hdr->dummy = 0x00;

    /* 0x0c: Bloodstained COTM (Vita/3DS), Mighty Gunvolt Burst (PC); 0x10: rest */
    if (hdr->head_size != 0x0c && hdr->head_size != 0x10)
        return false;
    /* same (no changes, after 0x34 is padding) */
    if (hdr->entry_size != 0x34 && hdr->entry_size != 0x40)
        return false;
    if (hdr->dummy != 0x00)
        return false;

    return true;
}

bool bigrp_entry_parse(bigrp_entry_t* entry, const uint8_t* buf)
{
    entry->codec = get_u32le(buf + 0x08);

    auto codec = static_cast<EntryCodec>(entry->codec);
    switch (codec)
    {
    case EntryCodec::Range:
    case EntryCodec::DCT:
    case EntryCodec::Data:
    default:
        return false;
    case EntryCodec::Midi:
        entry->body_offset = get_u32le(buf + 0x10);
        return true;
    }
}

} // namespace icelib
