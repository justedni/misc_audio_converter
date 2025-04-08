#include "inti_bigrp.h"

#include "inti_icelib.h"

#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <iostream>
#include <iomanip> 
#include <assert.h>
#include <bitset>
#include <tuple>
#include <filesystem>

#include "midi.h"
#include "wave.h"
#include "utils.h"

namespace Inti {

std::pair<std::string, std::string> get_current_song_name(const std::string& sequence_stem, bool bGroup)
{
    std::string common_stem = sequence_stem;
    assert(!common_stem.empty());
    if (common_stem.back() == '_')
        common_stem.pop_back();

    std::string song_name, track_name;
    if (bGroup)
    {
        std::size_t found = common_stem.find_last_of("_");
        if (found != std::string::npos)
        {
            song_name = common_stem.substr(0, found);
            track_name = common_stem.substr(found + 1, common_stem.size() - found - 1);
        }
        else
        {
            song_name = track_name = common_stem;
        }
    }
    else
    {
        song_name = track_name = common_stem;
    }

    return { song_name, track_name };
}

void bigrp_to_midi(std::string filepath, std::string out_folder, BigrpOptions& options)
{
    std::ifstream file(filepath, std::ios::in | std::ios::binary | std::ios::ate);
    if (!file.is_open())
        return;

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> buffer(fileSize);
    if (!file.read(buffer.data(), fileSize))
        return;

    auto* pData = (uint8_t*)buffer.data();

    icelib::bigrp_header_t header;
    if (!icelib::parse_bigrp_header(&header, pData, fileSize))
        return;

    fill_mappings_from_game_id(options);

    std::string bigrpName = std::filesystem::path(filepath).stem().string();

    GlobalMidiFile* mergedMidiFile = nullptr;

    if (options.bExportMergedMidis)
    {
        mergedMidiFile = new GlobalMidiFile(
            out_folder,
            (options.bPrefixWithBigrpName ? bigrpName : "")
        );
    }

    for (int iSong = 0; iSong < header.total_subsongs; iSong++)
    {
        int offset = header.head_size + header.entry_size * iSong;
        assert(offset < fileSize);

        const uint8_t* entryData = pData + offset;
        icelib::bigrp_entry_t entry;
        if (!icelib::bigrp_entry_parse(&entry, entryData))
            continue;

        auto codec = static_cast<icelib::EntryCodec>(entry.codec);

        switch (codec)
        {
        case icelib::EntryCodec::Range:
            break;
        case icelib::EntryCodec::Data:
            break;
        case icelib::EntryCodec::DCT:
            break;
        case icelib::EntryCodec::Midi:
        {
            uint32_t offsetInHeader = icelib::get_u32le(entryData + 0x10);
            uint32_t midiDataSize = icelib::get_u32le(entryData + 0x14);
            int midiDataStartOffset = header.head_size + (header.entry_size * iSong) + offsetInHeader;
            assert(midiDataStartOffset < fileSize);

            const uint8_t* pMidiData = pData + midiDataStartOffset;
            assert(strncmp((char*)pMidiData, "MThd", 4) == 0);

            std::string sequence_name = MidiUtils::get_midi_sequence_name(pMidiData, midiDataSize);

            std::string bigrp_stem = std::filesystem::path(filepath).stem().string();
            auto mappingFound = std::find_if(options.mappings.begin(), options.mappings.end(),
                [&bigrp_stem, &iSong](auto& e) { return e.bigrpFileName == bigrp_stem
                && iSong >= e.entryRange.first && iSong <= e.entryRange.second; });

            if (mappingFound != options.mappings.end())
            {
                sequence_name = mappingFound->songName;
            }

            if (options.bExportMidis)
            {
                std::string out_file_name;
                if (!sequence_name.empty())
                {
                    if (options.bPrefixWithBigrpName)
                        out_file_name = Utils::str_format("%s_%03d_%s", bigrpName.c_str(), iSong, sequence_name.c_str());
                    else
                        out_file_name = sequence_name;
                }
                else
                    out_file_name = Utils::str_format("%s_%03d", std::filesystem::path(filepath).stem().string().c_str(), iSong);

                std::string out_full_path = Utils::str_format("%s\\%s.mid", out_folder.c_str(), out_file_name.c_str());
                MidiUtils::write_raw_midi_file(out_full_path, pMidiData, midiDataSize);
            }

            if (options.bExportMergedMidis)
            {
                std::string out_file_name, track_name;
                if (!sequence_name.empty())
                {
                    if (mappingFound != options.mappings.end())
                    {
                        out_file_name = sequence_name;

                        assert(mergedMidiFile);
                        int fileId = mergedMidiFile->get_current_midi_id();
                        if (!mappingFound->trackNames.empty())
                        {
                            assert(mappingFound->trackNames.size() == 8);
                            track_name = mappingFound->trackNames[fileId];
                        }
                        else
                        {
                            track_name = MidiUtils::get_midi_last_track_name(pMidiData, midiDataSize);
                        }
                    }
                    else
                    {
                        std::tie(out_file_name, track_name) = get_current_song_name(sequence_name, true);
                    }
                }
                else
                {
                    out_file_name = Utils::str_format("%s_%03d", std::filesystem::path(filepath).stem().string().c_str(), iSong);
                    track_name = MidiUtils::get_midi_last_track_name(pMidiData, midiDataSize);
                }

                mergedMidiFile->add_midi_to_merge(pMidiData, midiDataSize, out_file_name, track_name);
            }
            break;
        }
        default:
            break;
        }
    }

    if (options.bExportMergedMidis)
    {
        assert(mergedMidiFile);
        delete mergedMidiFile;
    }
}

} // namespace Inti
