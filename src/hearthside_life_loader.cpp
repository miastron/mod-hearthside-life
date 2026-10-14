/* Hearthside Life loader; GPL-2.0-or-later, matching the modules. */

// AzerothCore derives this function's name from the module folder (modules/mod-hearthside-life), so
// the two must match. Life has no scripts of its own: it owns shared state and the headers that
// reach it. Its loader logs the registry's address, which City's and PvP's own startup lines repeat
// when they share it, and checks the reservation protocol like they do.

#include "LifeBotReservation.h"

#include "Log.h"

void Addmod_hearthside_lifeScripts()
{
    if (!PlayerbotsLife::CheckProtocolVersion())
        LOG_ERROR("server.loading", ">> Hearthside Life: reservation protocol {} does not match {} from a Life module. "
            "Rebuild every Life module against mod-hearthside-life.",
            PlayerbotsLife::LIFE_BOT_RESERVATION_PROTOCOL_VERSION, PlayerbotsLife::GetRegisteredProtocolVersion());
    LOG_INFO("server.loading", ">> Loaded mod-hearthside-life (protocol={}, registry={:p})",
        PlayerbotsLife::GetReservationProtocolVersion(), PlayerbotsLife::GetReservationRegistryAddress());
}
