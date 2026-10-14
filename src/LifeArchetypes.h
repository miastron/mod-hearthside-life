/* Hearthside Life archetype lookup; GPL-2.0-or-later, matching the modules. */
#ifndef LIFE_ARCHETYPES_H
#define LIFE_ARCHETYPES_H

// A slot mod-hearthside-chat fills with its archetype lookup, so City and PvP can pick bots by
// personality without including Chat's headers. Archetypes stay Chat's: their data, the GUID draw
// and the GM overrides. Without a provider (Chat not installed, or switched off) every lookup
// answers "unknown" and LifeBots::ArchetypeFilter lets every bot through.
//
// No AzerothCore types here: Chat includes this from its AzerothCore-facing files only, behind
// __has_include, and its standalone Tests/ never see it.

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace LifeArchetypes
{
    struct Provider
    {
        // The bot's archetype index (an HsArchetype value), or -1 if unknown. Callable from any thread.
        std::function<int(std::uint64_t guidRaw)> ForBot;
        // Comma-separated archetype names to a bitmask (1u << index); names it does not know go to
        // `unknown`. Callable from any thread.
        std::function<std::uint32_t(std::string const& csv, std::vector<std::string>& unknown)> MaskForNames;
        // Optional: false while the provider's module is switched off, which then counts as no provider.
        std::function<bool()> Enabled;
    };

    // At startup (the provider module's AddScripts). A second call replaces the first.
    void Register(Provider provider);
    // A provider is registered and switched on.
    bool Available();
    // Bumped by every Register, so a filter parsed before the provider arrived knows to parse again.
    std::uint32_t Generation();
    int ForBot(std::uint64_t guidRaw); // -1 without a provider
    std::uint32_t MaskForNames(std::string const& csv, std::vector<std::string>& unknown); // 0 without one
}

#endif
