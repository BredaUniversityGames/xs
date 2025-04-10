#pragma once

#include <string>

namespace xs::audio
{
	void initialize();
	void shutdown();
	void update(double dt);

	/// <summary>
	/// Loads a sound or music file into FMOD Core.
	/// </summary>
	/// <param name="filename">The name of the file to load.</param>
	/// <param name="group_id">The ID of the sound group to which this file should be added.</param>
	/// <returns>The ID of the sound that was loaded, or -1 if it failed to load.</returns>
	int load(const std::string& filename, int group_id);

	/// <summary>
	/// Plays a previously loaded FMOD Core sound or music file.
	/// </summary>
	/// <param name="sound_id">The ID of the sound to play.</param>
	/// <returns>The ID of the channel on which the sound is being played, 
	/// or -1 if the sound with the given ID does not exist.</returns>
	int play(int sound_id);

	/// <summary>
	/// Gets the current volume value of an FMOD Core sound group.
	/// </summary>
	/// <param name="group_id">The ID of the sound group for which you want to get the volume.</param>
	/// <returns>The current volume value (in the range 0-1) for the sound group, 
	/// or -1 if the group with the given ID does not exist.</returns>
	double get_group_volume(int group_id);

	/// <summary>
	/// Sets the volume value for an FMOD Core sound group.
	/// </summary>
	/// <param name="group_id">The ID of the sound group for which you want to set the volume.</param>
	/// <param name="value">The new volume value (in the range 0-1) for the sound group.</param>
	void set_group_volume(int group_id, double value);

	/// <summary>
	/// Gets the current volume multiplier of an FMOD Core sound channel, 
	/// relative to the general volume of the corresponding sound group.
	/// </summary>
	/// <param name="channel_id">The ID of the sound channel for which you want to get the volume.</param>
	/// <returns>The current volume multiplier (0 or higher) for the sound channel, 
	/// relative to the general volume of the corresponding sound group. 
	/// Returns -1 if the channel with the given ID does not exist.</returns>
	double get_channel_volume(int channel_id);

	/// <summary>
	/// Sets the volume multiplier for an FMOD Core sound channel, 
	/// relative to the general volume of the corresponding sound group.
	/// For example, setting it to 2 will make the sound on this channel play twice as loud as other sounds in the same group.
	/// </summary>
	/// <param name="channel_id">The ID of the sound channel for which you want to set the volume.</param>
	/// <param name="value">The new volume multiplier (0 or higher) for the sound channel.</param>
	void set_channel_volume(int group_id, double value);

	/// <summary>
	/// Gets the current volume of an FMOD Studio bus.
	/// </summary>
	/// <param name="name">The name of the bus for which you want to get the volume.</param>
	/// <returns>The current volume of the bus with the given name. 
	/// Returns -1 if a bus with this name does not exist.</returns>
	double get_bus_volume(const std::string& name);

	/// <summary>
	/// Sets the current volume of an FMOD Studio bus.
	/// </summary>
	/// <param name="name">The name of the bus for which you want to get she volume.</param>
	/// <param name="value">The new volume for the bus.</param>
	void set_bus_volume(const std::string& name, double value);

	/// <summary>
	/// Loads an FMOD Studio audio bank file.
	/// </summary>
	/// <param name="filename">The name of the file to load.</param>
	/// <returns>The ID of the bank that was loaded, or -1 if it failed to load.</returns>
	int load_bank(const std::string& filename);

	/// <summary>
	/// Unloads the FMOD Studio audio bank with the given ID.
	/// </summary>
	/// <param name="id">The ID of the bank to unload.</param>
	void unload_bank(int id);

	/// <summary>
	/// Starts a new instance of an FMOD Studio event with the given name.
	/// </summary>
	/// <param name="name">The name of the event to start.</param>
	/// <returns>The ID of the newly created event instance, or -1 if something went wrong, 
	/// for example if the given event name does not exist in any of the loaded banks.</returns>
	int start_event(const std::string& name);

	/// <summary>
	/// Updates the value a numeric FMOD Studio parameter (globally or for a specific event).
	/// </summary>
	/// <param name="eventID">The ID of the event to which the parameter is associated. 
	/// If you use -1, this function will affect a global parameter instead of an event parameter.</param>
	/// <param name="name">The name of the parameter to update.</param>
	/// <param name="value">The desired new value of the parameter.</param>
	void set_parameter_number(int eventID, const std::string& name, double value);

	/// <summary>
	/// Updates the value a labeled FMOD Studio parameter (globally or for a specific event).
	/// </summary>
	/// <param name="eventID">The ID of the event to which the parameter is associated. 
	/// If you use -1, this function will affect a global parameter instead of an event parameter.</param>
	/// <param name="name">The name of the parameter to update.</param>
	/// <param name="label">The desired new value of the parameter.</param>
	void set_parameter_label(int eventID, const std::string& name, const std::string& label);
}