#include "simple_audio.hpp"
#include "log.hpp"
#include "fileio.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <SDL3/SDL.h>

namespace xs::simple_audio
{

// Audio data structure
struct AudioData
{
	std::vector<uint8_t> buffer;
	SDL_AudioSpec spec;
};

// Playing channel structure
struct Channel
{
	SDL_AudioStream* stream = nullptr;
	int audio_id = -1;
	double volume = 1.0;
	bool playing = false;
};

// Internal state
namespace internal
{
	std::unordered_map<int, std::shared_ptr<AudioData>> audio_files;
	std::unordered_map<int, std::unique_ptr<Channel>> channels;
	SDL_AudioDeviceID device_id = 0;
	int next_audio_id = 1;
	int next_channel_id = 1;
	bool initialized = false;
}

void initialize()
{
	if (internal::initialized)
	{
		log::warn("SimpleAudio already initialized");
		return;
	}

	// Initialize SDL audio subsystem
	if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
	{
		log::error("Failed to initialize SDL audio subsystem: {}", SDL_GetError());
		return;
	}

	// Open the default audio playback device
	internal::device_id = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
	if (internal::device_id == 0)
	{
		log::error("Failed to open audio device: {}", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}

	// Resume the audio device (it starts paused by default)
	if (!SDL_ResumeAudioDevice(internal::device_id))
	{
		log::error("Failed to resume audio device: {}", SDL_GetError());
		SDL_CloseAudioDevice(internal::device_id);
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		return;
	}

	internal::initialized = true;
	log::info("SimpleAudio initialized using SDL3");
}

void shutdown()
{
	if (!internal::initialized)
		return;

	// Stop all channels and destroy streams
	for (auto& [id, channel] : internal::channels)
	{
		if (channel->stream)
		{
			SDL_DestroyAudioStream(channel->stream);
			channel->stream = nullptr;
		}
	}
	internal::channels.clear();

	// Clear audio data
	internal::audio_files.clear();

	// Close audio device
	if (internal::device_id)
	{
		SDL_CloseAudioDevice(internal::device_id);
		internal::device_id = 0;
	}

	// Quit SDL audio subsystem
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	internal::initialized = false;
	log::info("SimpleAudio shutdown");
}

void update(double dt)
{
	if (!internal::initialized)
		return;

	// Clean up finished channels
	std::vector<int> finished_channels;
	for (auto& [id, channel] : internal::channels)
	{
		if (channel->stream && channel->playing)
		{
			// Check if the stream has finished playing
			int queued = SDL_GetAudioStreamQueued(channel->stream);
			if (queued == 0)
			{
				channel->playing = false;
				finished_channels.push_back(id);
			}
		}
	}

	// Remove finished channels
	for (int id : finished_channels)
	{
		if (internal::channels[id]->stream)
		{
			SDL_DestroyAudioStream(internal::channels[id]->stream);
		}
		internal::channels.erase(id);
	}
}

int load(const std::string& filename)
{
	if (!internal::initialized)
	{
		log::error("SimpleAudio not initialized");
		return -1;
	}

	// Check if already loaded
	int hash = static_cast<int>(std::hash<std::string>{}(filename));
	if (internal::audio_files.find(hash) != internal::audio_files.end())
	{
		return hash;
	}

	// Read the audio file
	blob file_data = fileio::read_binary_file(filename);
	if (file_data.empty())
	{
		log::error("Failed to read audio file: {}", filename);
		return -1;
	}

	// Create SDL_IOStream from memory
	SDL_IOStream* io = SDL_IOFromConstMem(file_data.data(), file_data.size());
	if (!io)
	{
		log::error("Failed to create SDL_IOStream: {}", SDL_GetError());
		return -1;
	}

	// Load the audio file
	SDL_AudioSpec spec;
	Uint8* audio_buf = nullptr;
	Uint32 audio_len = 0;
		
	if (!SDL_LoadWAV_IO(io, true, &spec, &audio_buf, &audio_len))
	{
		log::error("Failed to load audio file {}: {}", filename, SDL_GetError());
		return -1;
	}

	// Store the audio data
	auto audio_data = std::make_shared<AudioData>();
	audio_data->spec = spec;
	audio_data->buffer.assign(audio_buf, audio_buf + audio_len);
	SDL_free(audio_buf);

	internal::audio_files[hash] = audio_data;
		
	log::info("Loaded audio file: {} (ID: {})", filename, hash);
	return hash;
}

/*
int play(int audio_id)
{
	return play(audio_id, 1.0);
}
*/

int play(int audio_id, double volume)
{
	if (!internal::initialized)
	{
		log::error("SimpleAudio not initialized");
		return -1;
	}

	// Find the audio data
	auto it = internal::audio_files.find(audio_id);
	if (it == internal::audio_files.end())
	{
		log::error("Audio ID {} not found", audio_id);
		return -1;
	}

	auto& audio_data = it->second;

	// Create an audio stream
	SDL_AudioStream* stream = SDL_CreateAudioStream(&audio_data->spec, &audio_data->spec);
	if (!stream)
	{
		log::error("Failed to create audio stream: {}", SDL_GetError());
		return -1;
	}

	// Bind the stream to the audio device
	if (!SDL_BindAudioStream(internal::device_id, stream))
	{
		log::error("Failed to bind audio stream: {}", SDL_GetError());
		SDL_DestroyAudioStream(stream);
		return -1;
	}

	// Set volume (clamp to 0.0 - 1.0)
	float sdl_volume = static_cast<float>(std::max(0.0, std::min(1.0, volume)));
	if (!SDL_SetAudioStreamGain(stream, sdl_volume))
	{
		log::warn("Failed to set audio stream gain: {}", SDL_GetError());
	}

	// Queue the audio data
	if (!SDL_PutAudioStreamData(stream, audio_data->buffer.data(), (int)audio_data->buffer.size()))
	{
		log::error("Failed to queue audio data: {}", SDL_GetError());
		SDL_DestroyAudioStream(stream);
		return -1;
	}

	// Flush the stream to start playback
	if (!SDL_FlushAudioStream(stream))
	{
		log::error("Failed to flush audio stream: {}", SDL_GetError());
		SDL_DestroyAudioStream(stream);
		return -1;
	}

	// Create and store the channel
	int channel_id = internal::next_channel_id++;
	auto channel = std::make_unique<Channel>();
	channel->stream = stream;
	channel->audio_id = audio_id;
	channel->volume = volume;
	channel->playing = true;

	internal::channels[channel_id] = std::move(channel);

	return channel_id;
}

void set_volume(int channel_id, double volume)
{
	if (!internal::initialized)
		return;

	auto it = internal::channels.find(channel_id);
	if (it == internal::channels.end() || !it->second->stream)
	{
		log::warn("Channel {} not found", channel_id);
		return;
	}

	// Clamp volume to 0.0 - 1.0
	volume = std::max(0.0, std::min(1.0, volume));
	it->second->volume = volume;

	float sdl_volume = static_cast<float>(volume);
	if (!SDL_SetAudioStreamGain(it->second->stream, sdl_volume))
	{
		log::warn("Failed to set audio stream gain: {}", SDL_GetError());
	}
}

double get_volume(int channel_id)
{
	if (!internal::initialized)
		return -1.0;

	auto it = internal::channels.find(channel_id);
	if (it == internal::channels.end())
	{
		return -1.0;
	}

	return it->second->volume;
}

void stop(int channel_id)
{
	if (!internal::initialized)
		return;

	auto it = internal::channels.find(channel_id);
	if (it == internal::channels.end())
		return;

	if (it->second->stream)
	{
		SDL_DestroyAudioStream(it->second->stream);
		it->second->stream = nullptr;
	}

	it->second->playing = false;
	internal::channels.erase(channel_id);
}

void stop_all()
{
	if (!internal::initialized)
		return;

	for (auto& [id, channel] : internal::channels)
	{
		if (channel->stream)
		{
			SDL_DestroyAudioStream(channel->stream);
			channel->stream = nullptr;
		}
		channel->playing = false;
	}

	internal::channels.clear();
}

bool is_playing(int channel_id)
{
	if (!internal::initialized)
		return false;

	auto it = internal::channels.find(channel_id);
	if (it == internal::channels.end())
		return false;

	return it->second->playing;
}

}
