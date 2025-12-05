#include "audio.hpp"

// Null audio implementation for platforms without FMOD support

namespace xs::audio
{
	void initialize() {}
	void shutdown() {}
	void update(double dt) {}

	int load(const std::string& filename, int group_id) { return -1; }
	int play(int sound_id) { return -1; }

	double get_group_volume(int group_id) { return -1; }
	void set_group_volume(int group_id, double value) {}

	double get_channel_volume(int channel_id) { return -1; }
	void set_channel_volume(int group_id, double value) {}

	double get_bus_volume(const std::string& name) { return -1; }
	void set_bus_volume(const std::string& name, double value) {}

	int load_bank(const std::string& filename) { return -1; }
	void unload_bank(int id) {}

	int start_event(const std::string& name) { return -1; }

	void set_parameter_number(int eventID, const std::string& name, double value) {}
	void set_parameter_label(int eventID, const std::string& name, const std::string& label) {}
}
