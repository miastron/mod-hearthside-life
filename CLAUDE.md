# mod-hearthside-life

Coordinator for the Hearthside Life modules. Plan and phase status: `PLAN-LIFE.md`.

## Build

No standalone build and no tests. The module compiles into AzerothCore's `modules` target at
`azerothcore-wotlk/modules/mod-hearthside-life/`; building, restarting and MySQL happen on the test
realm (see `../mod-hearthside-chat/CLAUDE.md`). Label any runtime claim you could not exercise as
unverified.

- The loader `Addmod_hearthside_lifeScripts()` (`src/hearthside_life_loader.cpp`) is named after the
  folder; renaming one means renaming the other.
- A new `.cpp` forces a CMake re-run on the realm and a full rebuild of `modules` (mod-playerbots
  included, 20-40 minutes). A new or changed header does not.

## Who uses what

- `mod-hearthside-city` and `mod-hearthside-pvp` include `../../mod-hearthside-life/src/<header>`
  and `#error` without this module. A change here changes both: build both.
- `mod-playerbots-artisans` (upstream, Ildourol) includes
  `../../mod-hearthside-city/src/LifeBotReservation.h`, which City keeps as a one-line forwarding
  header to this repo's. Do not patch Artisans.
- Upstream `mod-playerbots-city-life` commits that touch the reservation header or another shared
  `Life*.h` are ported here, not into City.

## Rules

- **Shared state lives in `src/LifeRegistry.cpp`,** behind the functions the headers declare
  (`PlayerbotsLife::GetReservationRegistry`, `LifeBots::HoldLevelsFor`, the `LifeArchetypes` provider
  slot, the `LifeActivity` busy marks, the `LifeEvents` listeners). Never add a `static` inside
  an `inline` header function for state the modules share: that only worked while every module
  compiled identical header text.
- Namespaces (`PlayerbotsLife`, `LifeBots`, `LifeMotion`, `LifeMedic`, `LifePresence`,
  `LifeArrival`, `LifeAudit`) are what City and PvP call; keep them.
- `LifeEvents.h`: `Kind` values are appended, never renumbered. `Publish` is world-thread only and
  catches a throwing listener (logged once per kind); listeners must only queue work. Events a
  listener already sees through its own hooks (duels) are never published here.
- Level holds (`LifeLevels.h`): holders are a `ReservationOwner` (City, PvP; `LifeBots::HoldLevels`)
  or `LevelHolderChat`. Each hands over its whole set; the union stays on mod-playerbots' exclude
  lists, and only names Life added come off, once nobody holds them. World thread only.
- `LifeArchetypes.h`, `LifeActivity.h`, `LifeEvents.h` and `LifeLevels.h` carry no AzerothCore types:
  Chat includes them behind `__has_include` from its AzerothCore-facing files. Keep them that way.
- `LIFE_BOT_RESERVATION_PROTOCOL_VERSION`: bump it on any change to the reservation structs or
  protocol. `CheckProtocolVersion()` is now only a startup sanity check.
- Logging: the `module` logger with a `[Life]` prefix (startup lines use `server.loading`).
- Modules build `static` on the realm. A `MODULES=dynamic` build is untested: City's and PvP's
  libraries would need Life's symbols at load time.
