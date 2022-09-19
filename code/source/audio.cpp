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
	FMOD::SoundGroup* group_sfx = nullptr;
	FMOD::SoundGroup* group_music = nullptr;

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

		core_system->createSoundGroup("sfx", &group_sfx);
		core_system->createSoundGroup("music", &group_music);
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

	void load(bool sound_effect, const std::string& filename)
	{
		const std::string& real_filename = fileio::get_path(filename);

		FMOD_MODE mode = sound_effect ? FMOD_DEFAULT : (FMOD_CREATESTREAM | FMOD_LOOP_NORMAL);
		FMOD::SoundGroup* group = sound_effect ? group_sfx : group_music;

		FMOD::Sound* sound;
		FMOD_RESULT result = core_system->createSound(real_filename.c_str(), mode, nullptr, &sound);
		if (result == FMOD_OK)
		{
			sound->setSoundGroup(group);
			sounds[filename] = sound;
		}
	}

	void play(const std::string& filename)
	{
		auto sound = sounds.find(filename);
		if (sound != sounds.end())
			core_system->playSound(sound->second, nullptr, false, nullptr);
	}

	double get_volume(bool sound_effects)
	{
		FMOD::SoundGroup* group = sound_effects ? group_sfx : group_music;
		float vol;
		group->getVolume(&vol);
		return static_cast<double>(vol);
	}

	void set_volume(bool sound_effects, double value)
	{
		FMOD::SoundGroup* group = sound_effects ? group_sfx : group_music;
		group->setVolume(static_cast<float>(value));
	}
}