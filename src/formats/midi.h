#pragma once

#include <string>
#include <vector>

namespace smf {
    class MidiEvent;
    class MidiFile;
}

namespace MidiUtils
{
    std::string get_midi_sequence_name(const uint8_t* buffer, int dataSize);
    std::string get_midi_last_track_name(const uint8_t* buffer, int dataSize);

    void write_raw_midi_file(const std::string& out_path, const uint8_t* pMidiData, uint32_t midiDataSize);
}

struct GlobalMidiFile
{
    GlobalMidiFile(const std::string& in_outFolder, const std::string& in_optionalBigrpName);
    ~GlobalMidiFile();

    void add_global_event(smf::MidiEvent& ev);

    void write_current_sequence();
    void create_new_sequence(const std::string& newSequenceName);
    int get_current_midi_id();
    void add_midi_to_merge(const uint8_t* buffer, int dataSize, const std::string& newSequenceName, const std::string& newTrackName);

private:
    const std::string outFolder;
    const std::string optionalBigrpName;

    int fileCount = 0;
    smf::MidiFile* smf = nullptr;
    std::string sequenceName;

    std::vector<smf::MidiEvent> globalEvents;
    bool bInitialized = false;
};
