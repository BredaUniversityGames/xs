#include "audio.hpp"
#include "fileio.hpp"
#include "log.hpp"
#include <fmod/inc/fmod.hpp>
#include <fmod/inc/fmod_studio.hpp>
#include <fmod/inc/fmod_errors.h>
#include <unordered_map>

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

	void load_fmod_library();	// implemented in platform-specific files as needed

	void addSoundGroup(const std::string& name, int group_id)
	{
		FMOD::SoundGroup* group;
		core_system->createSoundGroup(name.c_str(), &group);
		sound_groups[group_id] = group;
	}

	void initialize()
	{
		load_fmod_library();

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
		FMOD_MODE mode = FMOD_OPENMEMORY | FMOD_CREATESAMPLE;
		if (group_id == GROUP_MUSIC) mode = mode | FMOD_LOOP_NORMAL;
		FMOD::Sound* sound; 
		const blob& sound_data = fileio::read_binary_file(filename);
		FMOD_CREATESOUNDEXINFO info{};
		info.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
		info.length = (int)sound_data.size();
		FMOD_RESULT result = core_system->createSound(reinterpret_cast<const char*>(sound_data.data()), mode, &info, &sound);
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
		blob bank_data = fileio::read_binary_file(filename);
		if (bank_data.empty())
		{
			log::error("FMOD bank with filename {} could not be loaded!", filename);
			return -1;
		}
		auto result = system->loadBankMemory(
			reinterpret_cast<const char*>(bank_data.data()),
			(int)bank_data.size(),
			FMOD_STUDIO_LOAD_MEMORY,
			FMOD_STUDIO_LOAD_BANK_NORMAL,
			&bank);
		if (result != FMOD_OK)
		{
			// Get the error message
			const char* error = FMOD_ErrorString(result);
			log::error("FMOD bank with filename {} could not be loaded! Error: {}", filename, error);
			return -1;
		}

		// load all the bank's sample data immediately
		result =  bank->loadSampleData();
		if (result != FMOD_OK)
        {
            log::error("FMOD bank with filename {} could not load sample data!", filename);
            return -1;
        }

		result = system->flushSampleLoading(); // enable this to wait for loading to finish
		if (result != FMOD_OK)
        {
            log::error("FMOD bank with filename {} could not flush sample loading!", filename);
            return -1;
        }

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

#if !defined(PLATFORM_PS5)
void xs::audio::load_fmod_library() {} // Dummy function for non-PS5 platforms
#endif


