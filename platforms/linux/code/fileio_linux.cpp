#include "fileio.hpp"

#include <cstdlib>
#include <cassert>

#include "log.hpp"
#include "xs.hpp"

using namespace std;
using namespace xs;

string xs::fileio::internal::get_user_path()
{
	// XDG Base Directory specification
	// Priority: $XDG_CONFIG_HOME/xs, or ~/.config/xs as fallback
	const char* xdg_config = std::getenv("XDG_CONFIG_HOME");
	if (xdg_config != nullptr && xdg_config[0] != '\0')
		return string(xdg_config) + "/xs";

	const char* home = std::getenv("HOME");
	if (home != nullptr && home[0] != '\0')
		return string(home) + "/.config/xs";

	log::critical("Could not get HOME or XDG_CONFIG_HOME environment variable.");
	assert(false);
	return {};
}

string xs::fileio::internal::get_shared_path()
{
	if (xs::get_run_mode() == xs::run_mode::packaged)
		return {};

	return "resources";
}
