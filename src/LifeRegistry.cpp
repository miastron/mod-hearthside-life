/* Hearthside Life shared state; GPL-2.0-or-later, matching the modules. */

// Every piece of state the Life modules share lives here, once, behind the functions the headers
// declare. Before mod-hearthside-life these were statics inside inline functions, one definition only
// because every module compiled the same header text.

#include "LifeActivity.h"
#include "LifeArchetypes.h"
#include "LifeBotReservation.h"
#include "LifeBots.h"

#include <chrono>
#include <mutex>
#include <unordered_map>

namespace PlayerbotsLife
{
    ReservationRegistry& GetReservationRegistry()
    {
        static ReservationRegistry registry;
        return registry;
    }
}

namespace LifeBots
{
    void HoldLevels(PlayerbotsLife::ReservationOwner owner, std::unordered_set<std::string> const& names)
    {
        struct Registry
        {
            std::unordered_map<uint8, std::unordered_set<std::string>> Held;
            std::unordered_set<std::string> Added;
        };
        static Registry registry;
        registry.Held[static_cast<uint8>(owner)] = names;

        std::unordered_set<std::string> held;
        for (auto const& [holder, set] : registry.Held)
            held.insert(set.begin(), set.end());

        auto& brackets = sPlayerbotAIConfig.LevelBracketsExcludeNames;
        auto& resets = sPlayerbotAIConfig.ResetBotLevelExcludeNames;
        for (std::string const& name : held)
        {
            bool added = false;
            for (auto* list : { &brackets, &resets })
                if (!Detail::Has(*list, name))
                {
                    list->push_back(name);
                    added = true;
                }
            if (added)
                registry.Added.insert(name);
        }
        for (auto it = registry.Added.begin(); it != registry.Added.end();)
        {
            if (held.count(*it))
            {
                ++it;
                continue;
            }
            for (auto* list : { &brackets, &resets })
                list->erase(std::remove(list->begin(), list->end(), *it), list->end());
            it = registry.Added.erase(it);
        }
    }
}

namespace LifeArchetypes
{
    namespace
    {
        struct Slot
        {
            std::mutex Mutex;
            Provider Current;
            bool Registered = false;
            std::uint32_t Generation = 0;
        };

        Slot& GetSlot()
        {
            static Slot slot;
            return slot;
        }

        // A copy, so the provider is never called under the slot's lock.
        bool Take(Provider& out)
        {
            Slot& slot = GetSlot();
            std::lock_guard<std::mutex> lock(slot.Mutex);
            if (!slot.Registered)
                return false;
            out = slot.Current;
            return true;
        }
    }

    void Register(Provider provider)
    {
        Slot& slot = GetSlot();
        std::lock_guard<std::mutex> lock(slot.Mutex);
        slot.Current = std::move(provider);
        slot.Registered = slot.Current.ForBot && slot.Current.MaskForNames;
        ++slot.Generation;
    }

    bool Available()
    {
        Provider provider;
        return Take(provider) && (!provider.Enabled || provider.Enabled());
    }

    std::uint32_t Generation()
    {
        Slot& slot = GetSlot();
        std::lock_guard<std::mutex> lock(slot.Mutex);
        return slot.Generation;
    }

    int ForBot(std::uint64_t guidRaw)
    {
        Provider provider;
        if (!Take(provider) || (provider.Enabled && !provider.Enabled()))
            return -1;
        return provider.ForBot(guidRaw);
    }

    std::uint32_t MaskForNames(std::string const& csv, std::vector<std::string>& unknown)
    {
        Provider provider;
        if (!Take(provider))
            return 0;
        return provider.MaskForNames(csv, unknown);
    }
}

namespace LifeActivity
{
    namespace
    {
        using Clock = std::chrono::steady_clock;

        struct Marks
        {
            std::mutex Mutex;
            std::unordered_map<std::uint64_t, Clock::time_point> Until;
        };

        Marks& GetMarks()
        {
            static Marks marks;
            return marks;
        }
    }

    void Mark(std::uint64_t guidRaw, std::uint32_t durationMs)
    {
        if (!guidRaw)
            return;
        Marks& marks = GetMarks();
        std::lock_guard<std::mutex> lock(marks.Mutex);
        marks.Until[guidRaw] = Clock::now() + std::chrono::milliseconds(durationMs);
    }

    void Clear(std::uint64_t guidRaw)
    {
        Marks& marks = GetMarks();
        std::lock_guard<std::mutex> lock(marks.Mutex);
        marks.Until.erase(guidRaw);
    }

    bool Busy(std::uint64_t guidRaw)
    {
        Marks& marks = GetMarks();
        std::lock_guard<std::mutex> lock(marks.Mutex);
        auto it = marks.Until.find(guidRaw);
        if (it == marks.Until.end())
            return false;
        if (it->second > Clock::now())
            return true;
        marks.Until.erase(it);
        return false;
    }

    std::size_t Count()
    {
        Marks& marks = GetMarks();
        std::lock_guard<std::mutex> lock(marks.Mutex);
        Clock::time_point now = Clock::now();
        for (auto it = marks.Until.begin(); it != marks.Until.end();)
            it = it->second > now ? std::next(it) : marks.Until.erase(it);
        return marks.Until.size();
    }
}
