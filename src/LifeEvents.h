/* Hearthside Life event channel; GPL-2.0-or-later, matching the modules. */
#ifndef LIFE_EVENTS_H
#define LIFE_EVENTS_H

// City and PvP report what happened as plain facts; whoever listens (mod-hearthside-chat) decides
// whether anyone speaks. With no listener, Publish does nothing, so bots stay silent.
//
// Events a listener already sees through its own hooks are not published here: duels above all
// (Chat hooks every duel start and end, PvP's duel grounds included).
//
// No AzerothCore types here, for the same reason as LifeArchetypes.h.

#include "LifeBotReservation.h"

#include <cstdint>
#include <functional>
#include <string>

namespace LifeEvents
{
    // Appended, never renumbered.
    enum class Kind : std::uint16_t
    {
        CrowdPair          = 1, // City: two residents start a crowd scene near a real player
        RaidDefenseStarted = 2, // PvP: a reactive raid defense deploys
    };

    struct Event
    {
        Kind Type = Kind::CrowdPair;
        PlayerbotsLife::ReservationOwner From = PlayerbotsLife::ReservationOwner::CityLife;
        std::uint64_t Actor = 0, Other = 0; // raw GUIDs (ObjectGuid::GetRawValue())
        std::uint32_t MapId = 0;
        float X = 0.0f, Y = 0.0f, Z = 0.0f;
        std::uint64_t Witness = 0;          // the real player it happened near, if any
        std::string Detail;                 // e.g. the hub or zone name
    };

    using Listener = std::function<void(Event const&)>;

    // At startup (a module's AddScripts).
    void Listen(Listener listener);
    // World thread. Calls every listener in turn; one that throws is logged (once per kind) and
    // skipped, so a listener fault means silence, never a crash. Listeners must not block: queue the
    // work and do it on your own tick.
    void Publish(Event const& event);
    char const* KindName(Kind kind);
}

#endif
