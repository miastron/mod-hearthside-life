/* Hearthside Life shared state; GPL-2.0-or-later, matching the modules. */

// Every piece of state the Life modules share lives here, once, behind the functions the headers
// declare. Before mod-hearthside-life these were statics inside inline functions, one definition only
// because every module compiled the same header text.

#include "LifeBotReservation.h"
#include "LifeBots.h"

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
