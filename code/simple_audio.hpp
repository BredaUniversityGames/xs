#pragma once
#include <string>

namespace xs::simple_audio
{
	/// Initialize the simple audio system
	void initialize();

	/// Shutdown the simple audio system
	void shutdown();

	/// Update the audio system (called once per frame)
	void update(double dt);

	/// Load an audio file (WAV, FLAC, or OGG format)
	/// Returns an audio ID that can be used to play the sound, or -1 on error
	int load(const std::string& filename);

	/// Play a loaded audio file
	/// Returns a channel ID that can be used to control the sound, or -1 on error
	// int play(int audio_id);

	/// Play a loaded audio file with a specific volume
	/// Volume range: 0.0 (silent) to 1.0 (full volume)
	/// Returns a channel ID that can be used to control the sound, or -1 on error
	int play(int audio_id, double volume);

	/// Set the volume of a playing channel
	/// Volume range: 0.0 (silent) to 1.0 (full volume)
	void set_volume(int channel_id, double volume);

	/// Get the volume of a playing channel
	/// Returns volume in range 0.0 to 1.0, or -1 if channel doesn't exist
	double get_volume(int channel_id);

	/// Stop a playing channel
	void stop(int channel_id);

	/// Stop all playing channels
	void stop_all();

	/// Check if a channel is currently playing
	bool is_playing(int channel_id);
}
