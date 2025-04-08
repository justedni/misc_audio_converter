#include "inti_bigrp.h"

namespace Inti {

void fill_mappings_from_game_id(BigrpOptions& options)
{
    switch (options.game)
    {
    case EGame::Cotm1:
    {
        options.mappings = std::vector<Inti::Mapping>{ { "GROUP_TITLE", "BGM_FILE", { 12, 19 } } };
        break;
    }
    case EGame::Cotm2:
    {
        std::vector<std::string> regularTrackNameOrder = { "dpcm", "noise", "p1", "p2", "saw", "tri", "v1", "v2" };

        options.mappings = std::vector<Inti::Mapping>{
            { "GROUP_COMMON", "CM2_BGM_Unknown1", { 126, 133 }, regularTrackNameOrder },
            { "GROUP_COMMON", "RON8_JGL_PARTY_JOIN_2", { 150, 157 }, regularTrackNameOrder },
            { "GROUP_COMMON", "CM2_JGL_MAP", { 158, 165 }, regularTrackNameOrder },
            { "GROUP_COMMON", "CM2_BGM_Unknown2", { 186, 193 }, regularTrackNameOrder },
            { "GROUP_ST_01" , "CM2_BGM_ST01_2", { 258, 265 }, regularTrackNameOrder },
            { "GROUP_ST_10" , "CM2_BGM_DM06", { 308, 315 }, regularTrackNameOrder },
            { "GROUP_ST_10" , "CM2_BGM_DM06_2", { 316, 323 }, regularTrackNameOrder },
            { "GROUP_STG" , "CM2_BGM_STG", { 23, 30 }, regularTrackNameOrder },
        };
    }
    }
}

} // namespace Inti
