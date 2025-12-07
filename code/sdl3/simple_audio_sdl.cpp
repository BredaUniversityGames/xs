#include "simple_audio.hpp"
#include "log.hpp"
#include "fileio.hpp"
#include <SDL3/SDL.h>
#include <unordered_map>
#include <vector>
#include <algorithm>

namespace xs::simple_audio
{

struct AudioData
{
	Uint8* buffer = nullptr;
	Uint32 length = 0;
	SDL_AudioSpec spec;
};

struct Channel
{
	SDL_AudioStream* stream = nullptr;
	int audio_id = -1;
	double volume = 1.0;
	bool playing = false;
};

struct Data
{
	SDL_AudioDeviceID device = 0;
	SDL_AudioSpec device_spec;
	std::unordered_map<int, AudioData> loaded_audio;
	std::vector<Channel> channels;
	int next_audio_id = 0;
	int next_channel_id = 0;
};

static Data* data = nullptr;

void initialize()
{
	data = new Data();

	if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
	{
		log::error("Failed to initialize SDL audio: {}", SDL_GetError());
		return;
	}

	// Open the default audio device
	data->device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	if (!data->device)
	{
		log::error("Failed to open audio device: {}", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}

	// Get device spec
	SDL_GetAudioDeviceFormat(data->device, &data->device_spec, nullptr);

	log::info("SDL3 audio initialized: {} Hz, {} channels",
		data->device_spec.freq, data->device_spec.channels);
}

void shutdown()
{
	if (!data)
		return;

	// Stop all channels
	for (auto& channel : data->channels)
	{
		if (channel.stream)
		{
			SDL_DestroyAudioStream(channel.stream);
			channel.stream = nullptr;
		}
	}

	// Free all loaded audio
	for (auto& [id, audio] : data->loaded_audio)
	{
		if (audio.buffer)
			SDL_free(audio.buffer);
	}
	data->loaded_audio.clear();

	// Close audio device
	if (data->device)
		SDL_CloseAudioDevice(data->device);

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	delete data;
	data = nullptr;
}

void update(double dt)
{
	if (!data)
		return;

	// Clean up finished channels
	for (auto& channel : data->channels)
	{
		if (channel.playing && channel.stream)
		{
			// Check if stream has finished playing
			int queued = SDL_GetAudioStreamQueued(channel.stream);
			if (queued == 0)
			{
				channel.playing = false;
			}
		}
	}
}

int load(const std::string& filename)
{
	if (!data)
		return -1;

	std::string path = fileio::get_path(filename);

	SDL_AudioSpec spec;
	Uint8* buffer = nullptr;
	Uint32 length = 0;

	if (!SDL_LoadWAV(path.c_str(), &spec, &buffer, &length))
	{
		log::error("Failed to load audio file {}: {}", filename, SDL_GetError());
		return -1;
	}

	int audio_id = data->next_audio_id++;

	AudioData audio_data;
	audio_data.buffer = buffer;
	audio_data.length = length;
	audio_data.spec = spec;

	data->loaded_audio[audio_id] = audio_data;

	log::info("Loaded audio file {}: ID {}", filename, audio_id);
	return audio_id;
}

int play(int audio_id, double volume)
{
	if (!data || !data->device)
		return -1;

	auto it = data->loaded_audio.find(audio_id);
	if (it == data->loaded_audio.end())
	{
		log::error("Audio ID {} not found", audio_id);
		return -1;
	}

	const AudioData& audio = it->second;

	// Create audio stream for format conversion
	SDL_AudioStream* stream = SDL_CreateAudioStream(&audio.spec, &data->device_spec);
	if (!stream)
	{
		log::error("Failed to create audio stream: {}", SDL_GetError());
		return -1;
	}

	// Bind stream to the audio device
	SDL_BindAudioStream(data->device, stream);

	// Queue the audio data
	if (!SDL_PutAudioStreamData(stream, audio.buffer, audio.length))
	{
		log::error("Failed to queue audio data: {}", SDL_GetError());
		SDL_DestroyAudioStream(stream);
		return -1;
	}

	// Find an available channel or create a new one
	int channel_id = -1;
	for (size_t i = 0; i < data->channels.size(); i++)
	{
		if (!data->channels[i].playing)
		{
			// Destroy old stream if exists
			if (data->channels[i].stream)
			{
				SDL_DestroyAudioStream(data->channels[i].stream);
			}

			data->channels[i].stream = stream;
			data->channels[i].audio_id = audio_id;
			data->channels[i].volume = volume;
			data->channels[i].playing = true;
			channel_id = static_cast<int>(i);
			break;
		}
	}

	// If no available channel, create a new one
	if (channel_id == -1)
	{
		Channel channel;
		channel.stream = stream;
		channel.audio_id = audio_id;
		channel.volume = volume;
		channel.playing = true;
		channel_id = static_cast<int>(data->channels.size());
		data->channels.push_back(channel);
	}

	// Set volume (SDL uses 0.0 to 1.0 range which matches our API)
	SDL_SetAudioStreamGain(stream, static_cast<float>(volume));

	return channel_id;
}

void set_volume(int channel_id, double volume)
{
	if (!data || channel_id < 0 || channel_id >= static_cast<int>(data->channels.size()))
		return;

	Channel& channel = data->channels[channel_id];
	channel.volume = volume;

	if (channel.stream)
		SDL_SetAudioStreamGain(channel.stream, static_cast<float>(volume));
}

double get_volume(int channel_id)
{
	if (!data || channel_id < 0 || channel_id >= static_cast<int>(data->channels.size()))
		return -1.0;

	return data->channels[channel_id].volume;
}

void stop(int channel_id)
{
	if (!data || channel_id < 0 || channel_id >= static_cast<int>(data->channels.size()))
		return;

	Channel& channel = data->channels[channel_id];

	if (channel.stream)
	{
		SDL_ClearAudioStream(channel.stream);
		channel.playing = false;
	}
}

void stop_all()
{
	if (!data)
		return;

	for (auto& channel : data->channels)
	{
		if (channel.stream && channel.playing)
		{
			SDL_ClearAudioStream(channel.stream);
			channel.playing = false;
		}
	}
}

bool is_playing(int channel_id)
{
	if (!data || channel_id < 0 || channel_id >= static_cast<int>(data->channels.size()))
		return false;

	const Channel& channel = data->channels[channel_id];

	if (!channel.playing || !channel.stream)
		return false;

	// Check if stream still has audio queued
	int queued = SDL_GetAudioStreamQueued(channel.stream);
	return queued > 0;
}

} // namespace xs::simple_audio
