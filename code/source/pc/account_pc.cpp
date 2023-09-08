#include "account.h"
#include <steam/steam_api.h>

#include "pc/account_pc.h"

namespace xs::account::internal 
{
	bool steam_initialized = false;
}

void xs::account::initialize()
{
	auto is_running = SteamAPI_IsSteamRunning();
	if(is_running)
		internal::steam_initialized = SteamAPI_Init();
}

void xs::account::shutdown()
{
	SteamAPI_Shutdown();
}

bool xs::account::is_running_steam()
{
	return internal::steam_initialized;
}
