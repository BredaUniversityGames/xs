#pragma once

#include <string>

namespace xs::audio
{
	void initialize();
	void shutdown();
	void update(double dt);

	/// <summary>
	/// Loads a sound or music file.
	/// </summary>
	/// <param name="filename">The name of the file to load.</param>
	/// <param name="group_id">The ID of the sound group to which this file should be added.</param>
	/// <returns>The ID of the sound that was loaded.</returns>
	int load(const std::string& filename, int group_id);

	/// <summary>
	/// Plays a previously loaded sound or music file.
	/// </summary>
	/// <param name="sound_id">The ID of the sound to play.</param>
	/// <returns>The ID of the channel on which the sound is being played.</returns>
	int play(int sound_id);

	/// <summary>
	/// Gets the current volume value of a sound group.
	/// </summary>
	/// <param name="group_id">The ID of the sound group for which you want to get the volume.</param>
	/// <returns>The current volume value (in the range 0-1) for the sound group.</returns>
	double get_group_volume(int group_id);

	/// <summary>
	/// Sets the volume value for a sound group.
	/// </summary>
	/// <param name="group_id">The ID of the sound group for which you want to get the volume.</param>
	/// <param name="value">The new volume value (in the range 0-1) for the sound group.</param>
	void set_group_volume(int group_id, double value);
}