

#include "audio.h"
#include "fileio.h"
#include "log.h"
#include <fmod/fmod.hpp>
#include <fmod/fmod_studio.hpp>
#include <unordered_map>

#if defined(PLATFORM_PS5)
#include <kernel.h>
#endif

#ifndef PLATFORM_XBOX

namespace xs::audio
{
	FMOD::Studio::System* system = nullptr;
	FMOD::System* core_system = nullptr;

	std::unordered_map<int, FMOD::Sound*> sounds;
	std::unordered_map<int, FMOD::SoundGroup*> sound_groups; 

	std::unordered_map<int, FMOD::Studio::Bank*> banks;
	std::unordered_map<int, FMOD::Studio::EventInstance*> events;
	int nextEventID = 0;

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
#if defined(PLATFORM_PS5)
	#ifdef DEBUG
		SceKernelModule core_mod = sceKernelLoadStartModule("/app0/sce_module/libfmodL.prx", 0, NULL, 0, NULL, NULL);
		assert(core_mod >= 0);
		SceKernelModule studio_mod = sceKernelLoadStartModule("/app0/sce_module/libfmodstudioL.prx", 0, NULL, 0, NULL, NULL);
		assert(studio_mod >= 0);
	#else
		sceKernelLoadStartModule("/app0/sce_module/libfmod.prx", 0, NULL, 0, NULL, NULL);
		sceKernelLoadStartModule("/app0/sce_module/libfmodstudio.prx", 0, NULL, 0, NULL, NULL);
	#endif		
#endif
		log::info("Audio Engine: FMOD Studio by Firelight Technologies Pty Ltd.");

		// Create the Studio System object
		FMOD_RESULT result = FMOD::Studio::System::create(&system);
		if (result != FMOD_OK)
		{
			log::error("Failed to create the FMOD Studio System!");
			return;
		}

		// Initialize FMOD Studio, which will also initialize FMOD Core
		result = system->initialize(512, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0);
		if (result != FMOD_OK)
		{
			log::error("Failed to initialize the FMOD Studio System!");
			return;
		}

		// Get the Core System pointer from the Studio System object
		result = system->getCoreSystem(&core_system);
		if (result != FMOD_OK)
		{
			log::error("Failed to get the FMOD Studio System after initialization!");
			return;
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

		for (auto bank : banks)
			bank.second->unload();

		system->release();
	}

	void update(double dt)
	{
		system->update();

		// remove released events from the hashmap?
		std::vector<int> eventsToRemove;
		for (auto eventInstance : events)
		{
			FMOD_STUDIO_PLAYBACK_STATE state;
			eventInstance.second->getPlaybackState(&state);
			if (state == FMOD_STUDIO_PLAYBACK_STOPPED)
				eventsToRemove.push_back(eventInstance.first);
		}

		for (auto id : eventsToRemove)
			events.erase(id);
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

	double get_bus_volume(const std::string& name)
	{
		FMOD::Studio::Bus* bus;
		auto result = system->getBus(name.c_str(), &bus);
		if (result != FMOD_OK)
		{
			log::error("FMOD Studio Bus with name {} does not exist!", name);
			return -1;
		}

		float vol;
		bus->getVolume(&vol);
		return static_cast<double>(vol);
	}

	void set_bus_volume(const std::string& name, double value)
	{
		FMOD::Studio::Bus* bus;
		auto result = system->getBus(name.c_str(), &bus);
		if (result != FMOD_OK)
		{
			log::error("FMOD Studio Bus with name {} does not exist!", name);
			return;
		}

		bus->setVolume(static_cast<float>(value));
	}

	int load_bank(const std::string& filename)
	{
		// check if the bank already exists
		int hash = static_cast<int>(std::hash<std::string>{}(filename));
		if (banks.find(hash) != banks.end())
			return hash;
		
		// try to load the bank
		FMOD::Studio::Bank* bank;
		const std::string& real_filename = fileio::get_path(filename);
		auto result = system->loadBankFile(real_filename.c_str(), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank);
		if (result != FMOD_OK)
		{
			log::error("FMOD bank with filename {} could not be loaded!", filename);
			return -1;
		}

		// load all of the bank's sample data immediately
		bank->loadSampleData();
		system->flushSampleLoading(); // enable this to wait for loading to finish

		// store the bank by its ID
		banks[hash] = bank;

		return hash;
	}

	void unload_bank(int id)
	{
		if (banks.find(id) == banks.end())
		{
			log::error("FMOD bank with ID {} does not exist!", id);
			return;
		}

		banks[id]->unload();
		banks.erase(id);
	}

	int start_event(const std::string& name)
	{
		// get the event description
		FMOD::Studio::EventDescription* evd;
		auto result = system->getEvent(name.c_str(), &evd);
		if (result != FMOD_OK)
		{
			log::error("FMOD event with name {} does not exist!", name);
			return -1;
		}

		// create an event instance
		FMOD::Studio::EventInstance* evi;
		result = evd->createInstance(&evi);
		if (result != FMOD_OK)
		{
			log::error("FMOD event instance with name {} could not be created!", name);
			return -1;
		}

		int eventID = nextEventID;
		events[eventID] = evi;
		++nextEventID;

		// trigger the event
		result = evi->start();

		// mark it for release immediately
		result = evi->release();

		return eventID;
	}

	void set_parameter_number(int eventID, const std::string& name, double value)
	{
		if (eventID < 0)
		{
			system->setParameterByName(name.c_str(), (float)value);
		}
		else
		{
			auto it = events.find(eventID);
			if (it == events.end())
			{
				log::error("FMOD event with ID {} does not exist!", eventID);
				return;
			}

			it->second->setParameterByName(name.c_str(), (float)value);
		}
	}

	void set_parameter_label(int eventID, const std::string& name, const std::string& label)
	{
		if (eventID < 0)
		{
			system->setParameterByNameWithLabel(name.c_str(), label.c_str());
		}
		else
		{
			auto it = events.find(eventID);
			if (it == events.end())
			{
				log::error("FMOD event with ID {} does not exist!", eventID);
				return;
			}

			it->second->setParameterByNameWithLabel(name.c_str(), label.c_str());
		}
	}
}

#else

namespace xs::audio
{
    void addSoundGroup(const std::string& name, int group_id)
    {
    }

    void initialize()
    {
    }

    void shutdown()
    {
    }

    void update(double dt)
    {
    }

    int load(const std::string& filename, int group_id)
    {
        return 0;
    }

    int play(int sound_id)
    {
        return 0;
    }

    double get_group_volume(int group_id)
    {
        return 0.0;
    }

    void set_group_volume(int group_id, double value)
    {
    }

    double get_channel_volume(int channel_id)
    {
        return 0.0;
    }

    void set_channel_volume(int channel_id, double value)
    {
    }

    double get_bus_volume(const std::string& name)
    {
        return 0.0;
    }

    void set_bus_volume(const std::string& name, double value)
    {
    }

    int load_bank(const std::string& filename)
    {
        return 0;
    }

    void unload_bank(int id)
    {
    }

    int start_event(const std::string& name)
    {
        return 0;
    }

    void set_parameter_number(int eventID, const std::string& name, double value)
    {
    }

    void set_parameter_label(int eventID, const std::string& name, const std::string& label)
    {
    }
}

#endif



