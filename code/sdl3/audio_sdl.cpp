#include "audio.hpp"

// SDL3 audio implementation stub for the full audio system
// This is a placeholder until FMOD or a full-featured audio engine is integrated
// For now, use simple_audio for basic audio needs

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
