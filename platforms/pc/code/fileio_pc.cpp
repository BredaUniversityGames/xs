#include "fileio.hpp"

#include <cassert>
#include <filesystem>

#include "log.hpp"
#include "xs.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

using namespace std;
using namespace xs;

string xs::fileio::internal::get_user_path()
{
#ifdef _WIN32
	char* pValue;
	size_t len;
	_dupenv_s(&pValue, &len, "APPDATA");
	if (pValue != nullptr)
	{
		string path = string(pValue) + string("\\xs");
		free(pValue);
		return path;
	}

	log::critical("Could not get the APPDATA environment variable.");
	assert(false);
	return {};
#else
	// Fallback for non-Windows PC builds (e.g. Linux compiled with PLATFORM_PC)
	const char* home = std::getenv("HOME");
	if (home != nullptr && home[0] != '\0')
		return string(home) + "/.config/xs";

	log::critical("Could not get HOME environment variable.");
	assert(false);
	return {};
#endif
}

string xs::fileio::internal::get_shared_path()
{
	std::string local_resources = "resources";

	// Get executable directory for installed engine path
	std::string exe_dir;
#ifdef _WIN32
	char buffer[MAX_PATH];
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	std::string exe_path(buffer);
	exe_dir = exe_path.substr(0, exe_path.find_last_of("\\/"));
#else
	char buffer[PATH_MAX];
	ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
	if (len != -1)
	{
		buffer[len] = '\0';
		std::string exe_path(buffer);
		exe_dir = exe_path.substr(0, exe_path.find_last_of("/"));
	}
#endif

	std::string installed_resources = exe_dir + "/resources";

	// Priority: Check pwd-relative first (development), then exe-relative (installed)
	if (fs::exists(local_resources))
	{
		log::info("Using local resources: {}", fs::absolute(local_resources).string());
		return local_resources;
	}
	else if (fs::exists(installed_resources))
	{
		log::info("Using installed engine resources: {}", installed_resources);
		return installed_resources;
	}
	else
	{
		log::warn("Could not find shared resources folder! Tried: {} and {}",
			fs::absolute(local_resources).string(), installed_resources);
		return local_resources; // Set anyway as fallback
	}
}
