#include "audio.h"

#include <fmod/fmod.hpp>
#include <fmod/fmod_studio.hpp>
#include <unordered_map>

#include "fileio.h"

namespace xs::audio
{
	FMOD::Studio::System* system = nullptr;
	FMOD::System* core_system = nullptr;

	std::unordered_map<std::string, FMOD::Sound*> sounds;

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
	}

	void shutdown()
	{
		for (auto sound : sounds)
			sound.second->release();

		system->release();
	}

	void update(double dt)
	{
		system->update();
	}

	void load_sound(const std::string& filename, bool stream, bool looping)
	{
		const std::string& real_filename = fileio::get_path(filename);

		FMOD::Sound* sound;
		FMOD_MODE mode = FMOD_DEFAULT;
		if (stream) mode = mode | FMOD_CREATESTREAM;
		if (looping) mode = mode | FMOD_LOOP_NORMAL;
		FMOD_RESULT result = core_system->createSound(real_filename.c_str(), mode, nullptr, &sound);
		if (result == FMOD_OK)
			sounds[filename] = sound;
	}

	void play_sound(const std::string& filename)
	{
		auto sound = sounds.find(filename);
		if (sound == sounds.end())
		{
			load_sound(filename, false, false);
			sound = sounds.find(filename);
		}

		if (sound != sounds.end())
			core_system->playSound(sound->second, nullptr, false, nullptr);
	}

	/*void unload_sound(const std::string& filename)
	{
		auto sound = sounds.find(filename);
		if (sound != sounds.end())
		{
			sound->second->release();
			sounds.erase(sound);
		}
	}*/
}