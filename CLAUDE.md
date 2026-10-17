# mod-hearthside-life

Coordinator for the Hearthside modules: the shared `Life*.h` headers and the state behind them.
Plan and phase status: `PLAN-LIFE.md`.

## The Hearthside collection

| Module | Role |
|---|---|
| `mod-hearthside-life` | Coordinator (this repo). City and PvP require it. |
| `mod-hearthside-chat` | All bot speech. Plugs in through its `hs_life.*` (`__has_include`). |
| `mod-hearthside-city` | Bots as city residents (fork of `mod-playerbots-city-life`). |
| `mod-hearthside-pvp` | Bots in world PvP and duels (fork of `mod-playerbots-pvp-life`). |

## Build

No standalone build, no tests. Compiles into AzerothCore's `modules` target; building, restarting
and MySQL happen on the test realm (see `../mod-hearthside-chat/CLAUDE.md`). Label any runtime
claim you could not exercise as unverified.

- The loader `Addmod_hearthside_lifeScripts()` (`src/hearthside_life_loader.cpp`) is named after
  the folder; renaming one means renaming the other.
- A new `.cpp` forces a CMake re-run and a full rebuild of `modules` (20-40 minutes); a new or
  changed header does not.
- **A change here changes City and PvP** (they include `../../mod-hearthside-life/src/<header>` and
  `#error` without it): build both.

## Upstream baseline

The single record for all four Hearthside modules (the others point here). Realm builds core
`mod-playerbots/azerothcore-wotlk` `Playerbot` at `2a2211cd8f3d` and `mod-playerbots/mod-playerbots`
`master` at `79bd428115c8` (both 2026-10-09; local clones `../azerothcore-wotlk-pb`,
`../mod-playerbots`).

Each row is the upstream pair that module was last checked against. Moving the realm forward does
not move a row: bump a module's row only when that module has been checked on the new pair, and say
how (`build`, or `build + in game`). A row behind the realm is the exact range to review for it.
**Commit every change to this table in this repo right away** (its own commit; pushing is optional),
even when nothing else here changed: its git history is the record of past baselines.

| Module | Core | Playerbots | Verified | Date |
|---|---|---|---|---|
| life | `2a2211cd8f3d` | `79bd428115c8` | build | 2026-10-15 |
| chat | `2a2211cd8f3d` | `79bd428115c8` | build | 2026-10-15 |
| city | `2a2211cd8f3d` | `79bd428115c8` | build | 2026-10-15 |
| pvp | `2a2211cd8f3d` | `79bd428115c8` | build | 2026-10-15 |
| npc | `2a2211cd8f3d` | `79bd428115c8` | build | 2026-10-17 |

`tools/Get-UpstreamDrift.ps1 [-Module city] [-Commits] [-NoFetch]` (pwsh 7) fetches both clones and,
per row, counts commits to `origin/` and lists changed upstream files sharing a name with a header
the module includes (City and PvP inherit Life's), plus mod-playerbots `conf/`. A compile catches
API breaks for all four at once; the list is for behavior and config changes a compile won't show.
When a drift breaks something, note it where it bit (City's "Core API drift", Chat's notes).

## Rules

- **Shared state lives in `src/LifeRegistry.cpp`,** behind the functions the headers declare. Never
  put a `static` inside an `inline` header function for state modules share.
- Keep the namespaces City and PvP call (`PlayerbotsLife`, `LifeBots`, `LifeMotion`, `LifeMedic`,
  `LifePresence`, `LifeArrival`, `LifeAudit`).
- `LifeArchetypes.h`, `LifeActivity.h`, `LifeEvents.h` and `LifeLevels.h` carry no AzerothCore
  types: Chat includes them from AzerothCore-facing files. Keep them that way.
- `LifeEvents.h`: `Kind` values are appended, never renumbered. `Publish` is world-thread only and
  catches a throwing listener; listeners only queue work. Don't publish events listeners already
  see through their own hooks (duels).
- `LifeLevels.h`: holders (a `ReservationOwner` via `LifeBots::HoldLevels`, or `LevelHolderChat`)
  each hand over their whole set; Life removes only names it added, once nobody holds them. World
  thread only.
- Bump `LIFE_BOT_RESERVATION_PROTOCOL_VERSION` on any change to the reservation structs or protocol.
- Upstream `mod-playerbots-city-life` commits touching the reservation header or another shared
  `Life*.h` are ported here, not into City.
- Upstream `mod-playerbots-artisans` includes `../../mod-hearthside-city/src/LifeBotReservation.h`,
  which City forwards to this repo. Don't patch Artisans.
- Logging: the `module` logger with a `[Life]` prefix (startup lines on `server.loading`).
- Modules build `static`; `MODULES=dynamic` is untested.
