#pragma once

#include <string>
#include <vector>

namespace Inti {

    constexpr static char kBigrpTag[8] = { 0x0c, 0, 0, 0, 0x34, 0, 0 ,0 };

    // List of currently handled/tested games
    enum EGame : uint8_t
    {
        Unknown,
        Cotm1,
        Cotm2
    };

    struct Mapping
    {
        const std::string bigrpFileName;
        const std::string songName;
        const std::pair<int, int> entryRange;
        const std::vector<std::string> trackNames;
    };

    struct BigrpOptions
    {
        bool bPrefixWithBigrpName = false;
        bool bExportMidis = false;
        bool bExportMergedMidis = true;

        EGame game = EGame::Unknown;
        std::vector<Mapping> mappings;
    };

    void bigrp_to_midi(std::string filepath, std::string out_folder, BigrpOptions& options);

    void fill_mappings_from_game_id(BigrpOptions& options);

} // namespace Inti
