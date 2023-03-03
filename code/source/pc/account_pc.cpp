#include "account.h"

#include <steam/steam_api.h>

void xs::account::initialize()
{
	auto succes = SteamAPI_Init();
}

void xs::account::shutdown()
{
	SteamAPI_Shutdown();
}
