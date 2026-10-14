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

The shared state (the reservation registry and the level holds) is defined once, in
`src/LifeRegistry.cpp`.

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
