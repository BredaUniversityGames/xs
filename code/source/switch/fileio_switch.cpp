#include "fileio.h"
#include "log.h"
#include <cassert>
#include <nn/fs.h>
#include <nn/nn_Assert.h>
#include <nn/nn_Abort.h>
#include <nn/nn_Log.h>
#include <nn/account/account_ApiForApplications.h>
#include "account_switch.h"

using namespace std;
using namespace xs;

void fileio::initialize(/* const string& main_script*/)
{
	nn::Result result;
	size_t cacheSize = 0;

	char* cacheBuffer = nullptr;

	// Mounts the file system.
	// Mounting requires a cache buffer.
	{
		log::info("Mount Rom");

		// Gets the buffer size needed for the file system metadata cache.
		// No error handling is needed. An abort occurs within the library when getting fails.
		(void)nn::fs::QueryMountRomCacheSize(&cacheSize);

		cacheBuffer = new char[cacheSize];
		assert(cacheBuffer);

		// Mounts the file system.
		// Do not release the cache buffer until you unmount.
		result = nn::fs::MountRom("rom", cacheBuffer, cacheSize);

		assert(result.IsSuccess());
	}

	// Mount save data
	{
		log::info("Mount save data");

		// Get the user identifier from the account
		nn::account::Uid user = nn::account::InvalidUid;
		nn::account::GetUserId(&user, account::get_account_handle());

		// Create the selected user save data.
		// If data already exists, does nothing and returns nn::ResultSuccess.
		result = nn::fs::EnsureSaveData(user);
		if (nn::fs::ResultUsableSpaceNotEnough::Includes(result))
		{
			// Error handling when the application does not have enough memory is required for the nn::fs::EnsureSaveData() function.
			// The system automatically displays a message indicating that there was not enough capacity to create save data in the error viewer.
			// The application must offer options to cancel account selection and to return to the prior scene.
			NN_ABORT("Usable space not enough.\n");
		}

		// Mount the save data as "save."
		result = nn::fs::MountSaveData("save", user);
		// Always abort when a failure occurs.
		NN_ABORT_UNLESS_RESULT_SUCCESS(result);
	}

	add_wildcard("[games]", "rom:");
	add_wildcard("[save]", "save:");


	// All platforms
	bool success = false;
	if (exists("[games]/.ini"))
	{
		auto game_str = read_text_file("[games]/.ini");
		if (!game_str.empty())
		{
			string cwd = "[games]/" + game_str;
			if (exists(cwd + "/game.wren"))
			{
				cwd = get_path(cwd);
				add_wildcard("[game]", cwd);
				success = true;
			}
		}
	}

	if (!success)
		log::info("Please provide a valid game folder in the game name");
}