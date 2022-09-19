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
	/// <param name="sound_effect">Whether the file should be treated as a sound effect (true) or music (false).</param>
	/// <param name="filename">The name of the file to load.</param>
	void load(bool sound_effect, const std::string& filename);

	/// <summary>
	/// Plays a previously loaded sound or music file.
	/// </summary>
	/// <param name="filename">The name of the file to play.</param>
	void play(const std::string& filename);

	/// <summary>
	/// Gets the current volume value for sound effects or music.
	/// </summary>
	/// <param name="sound_effects">Whether to obtain the volume for sound effects (true) or for music (false).</param>
	/// <returns>The current volume value (in the range 0-1) for sound effects or music.</returns>
	double get_volume(bool sound_effects);

	/// <summary>
	/// Sets the volume value for sound effects or music.
	/// </summary>
	/// <param name="sound_effects">Whether to set the volume for sound effects (true) or for music (false).</param>
	/// <param name="value">The new volume value (in the range 0-1) for sound effects or music.</param>
	void set_volume(bool sound_effects, double value);
}