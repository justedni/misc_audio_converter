#include "midi.h"

#include <assert.h>
#include <filesystem>

#include "midifile/include/MidiFile.h"

#include "utils.h"

static constexpr int MIDI_META_SEQ_TRACK_NAME = 3;

GlobalMidiFile::GlobalMidiFile(const std::string& in_outFolder, const std::string& in_optionalBigrpName)
    : outFolder(in_outFolder)
    , optionalBigrpName(in_optionalBigrpName)
{}

GlobalMidiFile::~GlobalMidiFile()
{
    assert(fileCount == 0);
    assert(smf == nullptr);
}

void GlobalMidiFile::add_global_event(smf::MidiEvent& ev)
{
    auto found = std::find_if(globalEvents.begin(), globalEvents.end(), [&ev](auto& other)
    {
        if (ev.tick == other.tick && ev.getP0() == other.getP0())
        {
            if (ev.isMetaMessage())
                return (ev.getMetaContent() == other.getMetaContent());
            else
                return (ev.getP1() == other.getP1() && ev.getP2() == other.getP2());
        }
        return false;
    });

    if (found == globalEvents.end())
    {
        globalEvents.push_back(smf::MidiEvent(ev));
    }
}

void GlobalMidiFile::write_current_sequence()
{
    assert(smf);

    for (auto& ev : globalEvents)
    {
        smf->addEvent(0, ev);
    }

    smf->sortTracks();

    std::string out_path;
    if (!optionalBigrpName.empty())
        out_path = Utils::str_format("%s\\%s_%s.mid", outFolder.c_str(), optionalBigrpName.c_str(), sequenceName.c_str());
    else 
        out_path = Utils::str_format("%s\\%s.mid", outFolder.c_str(), sequenceName.c_str());

    bool bSuccess = smf->write(out_path);
    assert(bSuccess);

    globalEvents.clear();
    delete smf;
    smf = nullptr;
    sequenceName.clear();
}

void GlobalMidiFile::create_new_sequence(const std::string& newSequenceName)
{
    assert(smf == nullptr);
    assert(globalEvents.empty());

    sequenceName = newSequenceName;
    bInitialized = false;
    smf = new smf::MidiFile();
}

int GlobalMidiFile::get_current_midi_id()
{
    return fileCount;
}

void GlobalMidiFile::add_midi_to_merge(const uint8_t* buffer, int dataSize, const std::string& newSequenceName, const std::string& newTrackName)
{
    if (fileCount == 0)
    {
        assert(sequenceName != newSequenceName);
        create_new_sequence(newSequenceName);
    }
    else
    {
        assert(sequenceName == newSequenceName);
    }

    std::stringstream iss;
    iss.write((const char*)buffer, dataSize);

    smf::MidiFile midifile;
    midifile.read(iss);

    if (!bInitialized)
    {
        smf->setTPQ(midifile.getTPQ());
        bInitialized = true;
    }
    else
    {
        assert(smf->getTPQ() == midifile.getTPQ());
    }

    int currentChannel = -1;
    int globalTrackId = smf->addTrack();
    smf->addTrackName(globalTrackId, 0, newTrackName);

    int numTracks = midifile.getTrackCount();
    for (int track = 0; track < numTracks; track++)
    {
        int numEvents = midifile[track].size();
        for (int ev = 0; ev < numEvents; ev++)
        {
            auto& evData = midifile[track][ev];

            if (evData.isNoteOn())
            {
                if (currentChannel == -1)
                    currentChannel = evData.getChannel();
                else
                    assert(currentChannel == evData.getChannel());
            }

            bool bIgnoreMsg = (evData.isMetaMessage() && evData.getMetaType() == MIDI_META_SEQ_TRACK_NAME);

            if (track == 0) // meta-events track
            {
                if (!bIgnoreMsg)
                    add_global_event(evData);
            }
            else
            {
                if (evData.isMarkerText())
                {
                    add_global_event(evData);
                }
                else if (!bIgnoreMsg)
                {
                    if (evData.getChannel() == 9) // Fix bug with dpcm on midi chan 10: doesn't work with the sf2
                        evData.setChannel(11);

                    if (evData.isPitchbend())
                    {
                        const int lsb = evData.getP1();
                        const int msb = evData.getP2();
                        const int value = (msb << 7) | lsb;

                        evData.makePitchBend(evData.getChannel(), value);
                        smf->addEvent(globalTrackId, evData);
                    }
                    else
                    {
                        smf->addEvent(globalTrackId, evData);
                    }
                }
            }
        }
    }

    fileCount++;
    if (fileCount == 8)
    {
        write_current_sequence();
        fileCount = 0;
    }
}

namespace MidiUtils {

std::string get_midi_sequence_name(const uint8_t* buffer, int dataSize)
{
    std::stringstream iss;
    iss.write((const char*)buffer, dataSize);

    smf::MidiFile midifile;
    midifile.read(iss);

    auto findTrackName = [&](auto trackId)
    {
        int numEvents = midifile[trackId].size();
        for (int ev = 0; ev < midifile[trackId].size(); ev++)
        {
            auto& evData = midifile[trackId][ev];
            if (evData.isMetaMessage())
            {
                if (evData.getMetaType() == MIDI_META_SEQ_TRACK_NAME)
                {
                    std::string sequenceName = evData.getMetaContent();
                    if (!sequenceName.empty())
                        return sequenceName;
                }
            }
        }

        return std::string();
    };

    std::string sequenceName("");

    int numTracks = midifile.getTrackCount();
    if (numTracks > 0)
    {
        // Regular sequence, first track should contain the sequence name
        sequenceName = findTrackName(0);
        if (!sequenceName.empty())
        {
            if (sequenceName == "?") // Custom handling of unhandled char
            {
                sequenceName = "Unknown";
            }
        }
    }

    // No valid string was found: sequence name is unknown
    return sequenceName;
}

std::string get_midi_last_track_name(const uint8_t* buffer, int dataSize)
{
    std::stringstream iss;
    iss.write((const char*)buffer, dataSize);

    smf::MidiFile midifile;
    midifile.read(iss);

    int numTracks = midifile.getTrackCount();
    if (numTracks > 0)
    {
        int trackId = numTracks - 1;
        int numEvents = midifile[trackId].size();
        for (int ev = 0; ev < numEvents; ev++)
        {
            auto& evData = midifile[trackId][ev];
            if (evData.isMetaMessage())
            {
                if (evData.getMetaType() == MIDI_META_SEQ_TRACK_NAME)
                {
                    std::string sequenceName = evData.getMetaContent();
                    if (!sequenceName.empty())
                        return sequenceName;
                }
            }
        }
    }

    return "";
}

void write_raw_midi_file(const std::string& out_path, const uint8_t* pMidiData, uint32_t midiDataSize)
{
    bool bFileExists = std::filesystem::exists(out_path);
    std::ofstream os(out_path, std::ofstream::binary);
    os.write((char*)pMidiData, midiDataSize);
    os.close();
}

} // namespace MidiUtils
