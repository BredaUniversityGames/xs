#include "account.h"
#include "account_switch.h"

#include <nn/account/account_ApiForApplications.h>
#include <nn/account/account_Selector.h>
#include <nn/account/account_Result.h>
#include <nn/util/util_ScopeExit.h>


namespace xs::account::internal
{
	nn::account::UserHandle account_handle;
}

using namespace xs::account;

void xs::account::initialize()
{
	// Initialize the account library.
	nn::account::Initialize();

	nn::account::UserHandle userHandle{};

	if (TryOpenPreselectedUser(&userHandle))
	{
	}
	else
	{
		// Display the user account selection screen and make the user select a user account.
		// nn::account::ResultCancelledByUser is returned if the process is canceled by user operation.
		nn::account::Uid user{};
		const nn::Result result = ShowUserSelector(&user);
		if (nn::account::ResultCancelledByUser::Includes(result))
		{
			// Kill game?
			NN_ABORT("No account was selected.\n");
		}
		NN_ABORT_UNLESS_RESULT_SUCCESS(nn::account::OpenUser(&userHandle, user));
	}

	internal::account_handle = userHandle;
}

void xs::account::shutdown()
{
	CloseUser(get_account_handle());
}

nn::account::UserHandle& xs::account::get_account_handle()
{
	return internal::account_handle;
}

