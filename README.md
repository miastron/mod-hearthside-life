# mod-hearthside-life

The coordinator for the Hearthside Life modules. It has no gameplay of its own and no settings:
it holds the code and state that `mod-hearthside-city` (City Life), `mod-hearthside-pvp`
(PvP Life) and `mod-playerbots-artisans` share, so there is exactly one copy of each.

| Header | What it is |
| --- | --- |
| `LifeBotReservation.h` | Leases, admission, duel staging slots, real-player challenge cooldowns |
| `LifeMotion.h` | Validated jump/curve move; idle antics |
| `LifeBots.h` | Bot/human test, account and archetype filters, budget, census, strategy journal, level freeze |
| `LifeAudit.h` | Async audit writer (one per module) |
| `LifeArrival.h` | Out-of-sight drop-off, sight check, walk-off |
| `LifeMedic.h` | Healers tend finished duels |
| `LifePresence.h` | Real-player snapshot, mod-playerbots wake rule |
| `LifeArchetypes.h` | Archetype lookup that mod-hearthside-chat fills; without it every bot fits |
| `LifeActivity.h` | Busy marks: bots cast in a Chat conversation, left alone until it ends |
| `LifeEvents.h` | Event channel: City and PvP publish plain facts, Chat listens |
| `LifeLevels.h` | Level holds: the union of every holder's bots stays off mod-playerbots' re-leveling |

The shared state (the reservation registry, the level holds, the archetype provider and the busy
marks) is defined once, in `src/LifeRegistry.cpp`.

`mod-hearthside-chat` plugs in when it is installed: it registers the archetype provider and the
event listener, marks the bots it casts in a conversation, and holds its carded bots' levels as a
third holder beside City and PvP. City and PvP never include Chat's
headers. With no listener, publishing an event does nothing.

| Event | Sent by | When |
| --- | --- | --- |
| `CrowdPair` | City | Two residents start a crowd scene near a real player |
| `RaidDefenseStarted` | PvP | A reactive raid defense deploys |

## Installation

1. Place this module beside the Life modules that use it:
   ```text
   azerothcore-wotlk/modules/mod-hearthside-life
   ```
   City Life and PvP Life refuse to build without it.
2. Re-run CMake and rebuild AzerothCore. There is no conf file and no SQL.
3. Start worldserver and look for `>> Loaded mod-hearthside-life` in the startup log. City Life
   and PvP Life print the same `registry=` address when they share it.

`mod-playerbots-artisans` still includes `mod-hearthside-city/src/LifeBotReservation.h`; City keeps a
forwarding header there that leads here.

## License

GPL-2.0-or-later, see `LICENSE`.
