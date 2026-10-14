/* Hearthside Life activity marks; GPL-2.0-or-later, matching the modules. */
#ifndef LIFE_ACTIVITY_H
#define LIFE_ACTIVITY_H

// Who is busy talking. mod-hearthside-chat marks the bots it casts in a scripted or bot-to-bot
// conversation for as long as the scene runs; City and PvP check Busy before they relocate a bot,
// start an antic, pick it for something new or release it while idle, and simply wait. A mark
// always carries an expiry, so a scene that never clears its mark cannot pin a bot for good.
//
// Keyed by the raw player GUID (ObjectGuid::GetRawValue()). Callable from any thread.
// No AzerothCore types here, for the same reason as LifeArchetypes.h.

#include <cstddef>
#include <cstdint>

namespace LifeActivity
{
    // Busy for durationMs from now (a later Mark replaces the expiry).
    void Mark(std::uint64_t guidRaw, std::uint32_t durationMs);
    void Clear(std::uint64_t guidRaw);
    bool Busy(std::uint64_t guidRaw);
    // Marks not yet expired, for status lines.
    std::size_t Count();
}

#endif
