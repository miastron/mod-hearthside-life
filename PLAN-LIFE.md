# mod-hearthside-life plan

2026-10-14

A new `mod-hearthside-life` module becomes the coordinator for the Hearthside Life modules. It owns the shared code that City, PvP and Artisans use today from City's repo. It also gives Chat two registration points, an event listener and an archetype lookup, so City and PvP never call Chat directly. Archetypes stay in Chat. All bot speech moves to Chat; without Chat, bots stay silent.

## What moves

All seven City-owned shared headers move, about 2,850 lines. City keeps only its own code. Two of them also gain a `.cpp` in Life (see Coordinator services).

| Header | Lines | What it is | Used by today | In Life |
| --- | --- | --- | --- | --- |
| `LifeBotReservation.h` | 997 | Leases, admission, duel staging slots, real-player challenge cooldowns | City, PvP (a byte-identical copy), Artisans (City's copy) | One copy; registry state moves to `LifeRegistry.cpp` |
| `LifeMotion.h` | 657 | Validated jump/curve move; idle antics | City, PvP | Unchanged |
| `LifeBots.h` | 479 | Bot/human test, account and archetype filters, budget, census, strategy journal, level freeze | City, PvP | Level-freeze registry moves to `.cpp`; archetype filter asks the lookup instead of `hs_archetype.h` |
| `LifeAudit.h` | 218 | Async audit writer (one per module) | City, PvP | Unchanged |
| `LifeArrival.h` | 176 | Out-of-sight drop-off, sight check, walk-off | City, PvP | Unchanged |
| `LifeMedic.h` | 170 | Healers tend finished duels | City, PvP | Unchanged |
| `LifePresence.h` | 158 | Real-player snapshot, mod-playerbots wake rule | City, PvP | Unchanged |

New in Life: `LifeEvents.h` (the event channel), `LifeArchetypes.h` (the lookup slot) and `LifeActivity.h` (who is busy talking; see Coordinator services).

## At a glance

Today City is both a module and everyone's library. After the move, Life is the one thing the others depend on, and Chat plugs into it only when it is installed.

- **Today:**
  - PvP → City, for the shared headers; PvP `#error`s without City.
  - Artisans → City, for `LifeBotReservation.h`.
  - City and PvP → Chat, optionally, for `hs_archetype.h`.
  - Chat writes mod-playerbots' level lists on its own.
- **After:**
  - City → Life.
  - PvP → Life.
  - Artisans → City's forwarding header → Life.
  - Chat → Life, optionally: it registers the event listener, the archetype provider, activity marks and level holds.
  - City and PvP never reference Chat.

## Module layout

Life is a normal AzerothCore module at `modules/mod-hearthside-life`, in a new repo (`miastron/mod-hearthside-life`, to be created by you).

- **Files:** `src/` holds the headers above, plus `LifeRegistry.cpp` (all shared state) and `hearthside_life_loader.cpp`. The loader defines `Addmod_hearthside_lifeScripts()`; AzerothCore derives that name from the folder, so it must match. There is no `conf/` and no SQL at first, because Life has no settings of its own.
- **Includes:** City and PvP include `../../mod-hearthside-life/src/<header>` and `#error` without it, as PvP does for City today. Chat includes Life only behind `__has_include`, so Chat still builds and runs on a server without Life.
- **Artisans:** it is upstream (Ildourol) and looks for `../../mod-hearthside-city/src/LifeBotReservation.h`. City keeps a one-line forwarding header at that path that includes Life's, so Artisans needs no patch.
- **Namespaces:** unchanged (`PlayerbotsLife`, `LifeBots`, `LifeMotion`, `LifeMedic`, `LifePresence`, `LifeArrival`, `LifeAudit`), so moving the files touches only include lines. New code uses `LifeEvents`, `LifeArchetypes` and `LifeActivity`.
- **Logging:** the `module` logger with a `[Life]` prefix, matching `[CityLife]` and `[PvPLife]`.
- **Upstream ports:** City still reviews upstream `mod-playerbots-city-life` commits. Ones that touch the reservation header now land in Life instead of City; note this in City's `CLAUDE.md`.

## Coordinator services

Today the shared state lives in `static` variables inside inline functions. That works only because every module compiles the same header text. Life owns that state in one `.cpp` instead, behind the same function names, so there is one definition by construction.

| Service | Today | In Life | Thread rule |
| --- | --- | --- | --- |
| Reservations (leases, admission, duel slots, challenge cooldowns) | `GetReservationRegistry()` static in a header copied into two repos | `LifeRegistry.cpp`; the header keeps declarations and inline helpers | Unchanged: its own mutex, callable anywhere |
| Level holds (mod-playerbots' level-bracket exclude lists) | `LifeBots::HoldLevels` static; Chat writes the same lists on its own | `LifeRegistry.cpp`, with Chat as a third holder (see Chat changes) | World thread only, as now |
| Event channel | n/a | `LifeEvents`: listeners register, modules publish | Publish from the world thread; listeners must not block |
| Archetype lookup | City and PvP include `hs_archetype.h` | `LifeArchetypes`: Chat registers a provider | Callable anywhere; the provider must be thread-safe (Chat's already is) |
| Activity marks (who is busy talking) | n/a | `LifeActivity`: a bot GUID marked busy until a time | Callable anywhere (mutex) |

**Activity marks** close a gap that moving speech to Chat opens. Chat's scripted and bot-to-bot conversations pick two idle bots near a player and play turns over a minute or more. If either is a City resident, City may relocate it or start an antic mid-line. Chat marks both bots busy for the scene; it already tracks this as `Hs_IsBotInAnyScriptRun`. City and PvP check `LifeActivity::Busy(guid)` before relocating, starting an antic or releasing an idle bot, and simply wait.

The reservation protocol version stays as a constant. `CheckProtocolVersion()` can then only disagree if someone builds against an old City copy, so it becomes a startup sanity log rather than a cross-repo rule.

## Event channel

City and PvP report what happened as plain facts; whoever listens decides whether anyone speaks. With no listener, publishing does nothing, so bots stay silent.

```cpp
namespace LifeEvents
{
    enum class Kind : uint16 { CrowdPair = 1, RaidDefenseStarted = 2 }; // appended, never renumbered
    struct Event
    {
        Kind Type;
        PlayerbotsLife::ReservationOwner From; // CityLife or PvPLife
        uint64 Actor = 0, Other = 0;            // raw GUIDs
        uint32 MapId = 0; float X = 0, Y = 0, Z = 0;
        uint64 Witness = 0;                     // the real player it happened near, if any
        std::string Detail;                     // e.g. the hub or zone name
    };
    using Listener = std::function<void(Event const&)>;
    void Listen(Listener listener);   // at startup (a module's AddScripts)
    void Publish(Event const& event); // world thread; no listener = no-op
}
```

- `Publish` calls each listener inside `try`/`catch`. An exception is logged once per kind and swallowed, so a Chat fault means silence, never a crash. Chat's own listener only queues work, as all its hooks do today.
- Events Chat already sees through its own hooks are **not** republished. Duels are the main case: Chat hooks every duel start and end, including PvP's duel grounds.

| Event | Sent by | When | Chat's handling |
| --- | --- | --- | --- |
| `CrowdPair` | City | Two residents start a crowd scene near a real player | A scripted conversation or bot-to-bot chain is offered to that pair first |
| `RaidDefenseStarted` | PvP | A reactive raid defense deploys | Ignored until the dataset has rows for it, then a new Chat event type |
| Duel start/won/lost | (nobody) | Any duel | Chat's existing duel hooks, unchanged |

## Archetype lookup

Archetypes stay in Chat. Chat owns their data (`hside_archetype`), the GUID draw and everything the model uses. Life only holds a slot that Chat fills, so City and PvP ask Life and never include Chat's headers.

```cpp
namespace LifeArchetypes
{
    struct Provider
    {
        std::function<int(uint64 guidRaw)> ForBot;   // archetype index, or -1 if unknown
        std::function<uint32(std::string const& csv, std::vector<std::string>& unknown)> MaskForNames;
    };
    void Register(Provider provider); // Chat, at startup
    bool Available();
    int ForBot(uint64 guidRaw);       // -1 without a provider
    uint32 MaskForNames(std::string const& csv, std::vector<std::string>& unknown);
}
```

- `LifeBots::ArchetypeFilter` keeps its current behaviour. An empty list or no provider means every bot fits. A list that names nothing known means no bot fits, and is logged.
- **Ordering:** Chat registers in its `AddScripts`, before any module reads config. Its archetype table loads from the database later at startup. City and PvP borrow nothing during their 90-second startup delay, so no lookup runs before the table is there. If the list is parsed before Chat registers (a different load order), the filter re-parses once the provider appears.
- `Hs_ArchetypeForBot` and `Hs_ArchetypeMaskForNames` stay exactly as they are, and the provider wraps them. No bot changes archetype, and the fine-tune is untouched.
- The `LIFEBOTS_HAS_ARCHETYPES` include check goes away: the filter no longer needs Chat at build time.

## Chat changes

Every Chat change sits behind `__has_include("../../mod-hearthside-life/src/LifeEvents.h")`. Without Life, Chat builds and behaves exactly as today. Nothing here touches the prompt shape the fine-tune expects.

1. **Converter** (`hs_life.cpp`, new): registers the event listener and the archetype provider. `CrowdPair` becomes an offer to the existing scene runner (`hs_script.cpp`) or bot-to-bot chain (`hs_botchain.cpp`) for that pair, using the scripts and prompts they already have. `RaidDefenseStarted` is dropped until its dataset rows and event type exist.
2. **Activity marks:** when a scene casts a bot, Chat marks it busy in `LifeActivity` until the scene ends or aborts.
3. **Level holds:** today `hs_identity_store.cpp` pushes and erases carded bots' names on the exclude lists itself. Erasing can drop a name that City or PvP still holds, until their next tick puts it back. With Life present, Chat hands its set to Life as a third holder instead; without Life it keeps writing the lists as now.
4. **Bot detection** (optional, last): `Hs_IsBot` and `LifeBots::IsBotSession` run the same account test. Chat can delegate to Life's when it is present. `hs_bot.h` stays outside the code Chat's standalone `Tests/` compile, so this costs Chat's tests nothing.

A new `.cpp` in Chat means a CMake re-run on the realm (see Risks).

## Speech removed from City and PvP

All of it is off by default today, so a default server hears no difference. Body language stays: pairing, facing, emotes, AFK flags and cheering spectators.

| Module | Removed | Replaced by |
| --- | --- | --- |
| City | Scripted two-bot crowd dialogues (`CityLifeDialogues.h`, the `Say` turns in the crowd scene) | `CrowdPair` event; Chat's scripts and bot-to-bot chains |
| City | Inn and plaza greetings, random plaza `/say` | Chat's reflex replies when a player speaks; otherwise nothing |
| City | "Trading is disabled." whisper | Nothing (the trade is still blocked) |
| City | `CityLife.Chat.*` keys (`ScriptedConversations`, `Greetings`, `Unrestricted`, `RandomSayChance`, `SayCooldownSec`) | Removed from the conf |
| PvP | Skirmish and faction-campaign yells, world-channel lines, the chat queue (`ScheduleEventChat`, `ProcessChatQueue`) | Nothing |
| PvP | `pvp_life_chat` table, its startup check, the zone `bot_chat` flag and `.pvplife zone chat` | `DROP TABLE IF EXISTS pvp_life_chat` in the base SQL (safe to re-run); the `bot_chat` column stays unread so old rows still load |
| PvP | `PvPLife.BotChat.*` keys | Removed from the conf |

Both modules then always strip mod-playerbots' own `chat` and `emote` strategies from borrowed bots; today that depends on their chat settings being off. The only voice a borrowed bot has is then Chat's. `PvPLife.Announce` (GM event notices) is not bot speech and stays.

## Phases

Five phases, each with its own commits and realm build. Every install and restart waits for your go-ahead.

1. **Create Life and move the headers (no behaviour change).**
   - The new repo gets the seven headers, the loader and `LifeRegistry.cpp`, with the reservation and level-hold state moved out of the inline statics.
   - City and PvP include from Life. City keeps the forwarding header for Artisans. City's and PvP's copies of `LifeBotReservation.h` are deleted.
   - Both `CLAUDE.md` files and the memory notes are updated.
   - Gate:
     - The realm CMake re-run and full rebuild pass.
     - The startup log shows City, PvP and Artisans on the one registry.
     - `.citylife status` and `.pvplife reservations` look as before.
     - Residents and duel grounds run normally for an evening.
2. **Archetype lookup and activity marks.**
   - Life gets `LifeArchetypes` and `LifeActivity`, and `ArchetypeFilter` asks the lookup.
   - City and PvP respect busy marks.
   - Chat registers its provider. This is the first Chat change, behind `__has_include`.
   - Gate:
     - PvP's default archetype list still borrows the same kinds of bot (check `.pvplife scan`).
     - With Chat removed from a test build, every bot fits.
3. **Event channel and the Chat converter.**
   - Life gets `LifeEvents`, and City publishes `CrowdPair`.
   - Chat listens, offers the pair to its scene runner or bot-to-bot chain, and marks both bots busy.
   - Gate:
     - In Stormwind or Ironforge, crowd pairs near you hold a Chat conversation.
     - Nothing breaks when the model host is down.
4. **Remove City and PvP speech.**
   - Everything in the table above, conf entries included, plus the SQL drop.
   - Gate:
     - A server with Chat disabled is silent.
     - With Chat enabled, speech comes only from Chat (check the Chat conversation log).
5. **Chat as a level holder, and bot detection.**
   - Chat hands its carded bots to Life's level holds.
   - Optionally, `Hs_IsBot` delegates to Life.
   - Gate:
     - A carded bot that City borrows and Chat retires keeps its level.
     - Chat's `Tests/` still build standalone.

`RaidDefenseStarted` goes live whenever its dataset rows exist; it needs no other phase.

## Risks

| Risk | Handling |
| --- | --- |
| A new module and new `.cpp` files force a CMake re-run on the realm, which rebuilds all of mod-playerbots (20-40 minutes) | Phase 1 adds every Life `.cpp` at once, and Chat's `hs_life.cpp` arrives in phase 2. Budget one long build each time |
| Moving registry state out of headers changes which code owns it while bots are leased | Done at a restart, with nothing leased; behaviour is the same functions with one definition |
| Load order: a module reads config before Chat registers its archetype provider | Providers register in `AddScripts`, before any config load; the filter re-parses if the provider appears later |
| A listener fault in Chat takes City or PvP down with it | `Publish` catches and logs; listeners only queue work |
| City relocates a bot mid-conversation | Activity marks (phase 2) land before conversations are routed (phase 3) |
| Artisans is a third-party repo | City's forwarding header means no patch to it |
| Chat's standalone `Tests/` break | Life is only included from Chat's AzerothCore-facing files, never from the pure-logic ones the tests compile |
| Removing speech changes a server that had City or PvP chat switched on | Only operators who enabled those off-by-default keys notice; called out in both READMEs |

## Decisions (2026-10-14)

- [x] Repo: `miastron/mod-hearthside-life`.
- [x] Life stays settings-free until it needs a setting.
- [x] `pvp_life_chat` is dropped by PvP's base SQL.
- [x] A `CrowdPair` offer is enough for now; "Chat asks Life for two idle residents" waits until Chat's own scan is seen coming up empty.
- [x] Chat gets `HearthsideChat.Life.Enable` (default 1): off, City and PvP act as if Chat were not installed, so the phase 2 "without Chat" check needs no second build.
- [x] Bot detection (phase 5, optional) is not delegated: Life's `IsBotSession` needs a module's account filter, and Life has no settings of its own to hold one.
- [x] CrowdPair goes to the /say script runner only. The bot-to-bot chain (`hs_botchain.cpp`) is a party/raid/General mechanism seeded by a delivered line, not a /say pair.
