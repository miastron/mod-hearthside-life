/* Hearthside Life level holds; GPL-2.0-or-later, matching the modules. */
#ifndef LIFE_LEVELS_H
#define LIFE_LEVELS_H

// mod-playerbots' level brackets re-level a random bot in place. Each holder hands over the names of
// the bots it wants left at their level, every time its set changes (and an empty set to let go); the
// union of all holders is kept on mod-playerbots' two exclude lists
// (LevelBracketsExcludeNames/ResetBotLevelExcludeNames). Only names Life put there are ever taken
// back off, and only once no holder holds them, so one holder letting go never drops a name another
// still needs. World thread only: mod-playerbots reads those lists from it without a lock.
//
// City and PvP call LifeBots::HoldLevels with their ReservationOwner (LifeBots.h). Chat holds its
// carded bots under LevelHolderChat. No AzerothCore types here, for the same reason as
// LifeArchetypes.h.

#include <cstdint>
#include <string>
#include <unordered_set>

namespace LifeBots
{
    // Holder ids: a PlayerbotsLife::ReservationOwner value (0-2) for the Life modules, or one of these.
    constexpr std::uint8_t LevelHolderChat = 0x40;

    void HoldLevelsFor(std::uint8_t holder, std::unordered_set<std::string> const& names);
}

#endif
