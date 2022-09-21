#include "audio.h"

#include <fmod/fmod.hpp>
#include <fmod/fmod_studio.hpp>
#include <unordered_map>

#include "fileio.h"
#include "log.h"

namespace xs::audio
{
	FMOD::Studio::System* system = nullptr;
	FMOD::System* core_system = nullptr;

	std::unordered_map<int, FMOD::Sound*> sounds;
	std::unordered_map<int, FMOD::SoundGroup*> sound_groups; 

	const int GROUP_SFX = 1;
	const int GROUP_MUSIC = 2;

	void addSoundGroup(const std::string& name, int group_id)
	{
		FMOD::SoundGroup* group;
		core_system->createSoundGroup(name.c_str(), &group);
		sound_groups[group_id] = group;
	}

	void initialize()
	{
		// Create the Studio System object
		FMOD_RESULT result = FMOD::Studio::System::create(&system);
		if (result != FMOD_OK)
		{
			// TODO
		}

		// Initialize FMOD Studio, which will also initialize FMOD Core
		result = system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
		if (result != FMOD_OK)
		{
			// TODO
		}

		// Get the Core System pointer from the Studio System object
		system->getCoreSystem(&core_system);
		if (result != FMOD_OK)
		{
			// TODO
		}

		addSoundGroup("sfx", GROUP_SFX);
		addSoundGroup("music", GROUP_MUSIC);
	}

	void shutdown()
	{
		for (auto group : sound_groups)
			group.second->release();

		for (auto sound : sounds)
			sound.second->release();

		system->release();
	}

	void update(double dt)
	{
		system->update();
	}

	int load(const std::string& filename, int group_id)
	{
		// check if the sound already exists
		int hash = static_cast<int>(std::hash<std::string>{}(filename));
		if (sounds.find(hash) != sounds.end())
			return hash;

		// check if the group ID is valid; default to SFX group
		if (sound_groups.find(group_id) == sound_groups.end())
		{
			log::error("Sound group with ID {} does not exist!", group_id);
			return -1;
		}
		
		// try to load the sound file
		FMOD_MODE mode = (group_id == GROUP_SFX) ? FMOD_DEFAULT : (FMOD_CREATESTREAM | FMOD_LOOP_NORMAL);
		FMOD::Sound* sound; 
		const std::string& real_filename = fileio::get_path(filename);
		FMOD_RESULT result = core_system->createSound(real_filename.c_str(), mode, nullptr, &sound);
		if (result != FMOD_OK)
		{
			log::error("Sound with filename {} could not be loaded!", filename);
			return -1;
		}

		// attach the sound to the right group, and store it by its ID
		sound->setSoundGroup(sound_groups[group_id]);
		sounds[hash] = sound;
		
		return hash;
	}

	int play(int sound_id)
	{
		// check if the sound exists
		auto sound = sounds.find(sound_id);
		if (sound == sounds.end())
		{
			log::error("Sound with ID {} does not exist!", sound_id);
			return -1;
		}

		// play it, and retrieve the index of the channel on which it plays
		FMOD::Channel* channel;
		core_system->playSound(sound->second, nullptr, false, &channel);
		int channel_index; 
		channel->getIndex(&channel_index);
			
		return channel_index;
	}

	double get_group_volume(int group_id)
	{
		// check if the sound group exists
		auto group = sound_groups.find(group_id);
		if (group == sound_groups.end())
		{
			log::error("Sound group with ID {} does not exist!", group_id);
			return -1;
		}

		float vol;
		group->second->getVolume(&vol);
		return static_cast<double>(vol);
	}

	void set_group_volume(int group_id, double value)
	{
		// check if the sound group exists
		auto group = sound_groups.find(group_id);
		if (group == sound_groups.end())
		{
			log::error("Sound group with ID {} does not exist!", group_id);
			return;
		}

		group->second->setVolume(static_cast<float>(value));
	}

	double get_channel_volume(int channel_id)
	{
		FMOD::Channel* channel;
		auto result = core_system->getChannel(channel_id, &channel);
		if (result != FMOD_OK)
		{
			log::error("Sound channel with ID {} does not exist!", channel_id);
			return -1;
		}

		float vol;
		channel->getVolume(&vol);
		return static_cast<double>(vol);
	}

	void set_channel_volume(int channel_id, double value)
	{
		FMOD::Channel* channel;
		auto result = core_system->getChannel(channel_id, &channel);
		if (result != FMOD_OK)
		{
			log::error("Sound channel with ID {} does not exist!", channel_id);
			return;
		}

		channel->setVolume(static_cast<float>(value));
	}
}