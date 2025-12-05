#include "simple_audio.hpp"

// Null simple_audio implementation for platforms without audio support

namespace xs::simple_audio
{
	void initialize() {}
	void shutdown() {}
	void update(double dt) {}

	int load(const std::string& filename) { return -1; }
	int play(int sound_id, double volume) { return -1; }
	void stop(int sound_id) {}
	void stop_all() {}
	bool is_playing(int sound_id) { return false; }
	void set_volume(int sound_id, double volume) {}
	double get_volume(int sound_id) { return 0.0; }
}
