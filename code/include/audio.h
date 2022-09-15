#pragma once

#include <string>

namespace xs::audio
{
	void initialize();
	void shutdown();
	void update(double dt);

	void load_sound(const std::string& filename, bool stream, bool looping);
	void play_sound(const std::string& filename);
	//void unload_sound(const std::string& sound_file);
}