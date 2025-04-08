#pragma once

#include <cstdint>

namespace icelib {

enum class EntryCodec : uint8_t
{
    Range = 0x00,
    Data = 0x01,
    Midi = 0x02,
    DCT = 0x03,
};

struct bigrp_entry_t
{
    uint32_t codec;

    uint32_t body_offset;
};

struct bigrp_header_t
{
    uint32_t head_size;
    uint32_t entry_size;
    int total_subsongs;
    uint32_t dummy;
};

uint32_t get_u32le(const uint8_t* mem);

bool parse_bigrp_header(bigrp_header_t* hdr, const uint8_t* buf, int buf_size);
bool bigrp_entry_parse(bigrp_entry_t* entry, const uint8_t* buf);

} // namespace icelib
