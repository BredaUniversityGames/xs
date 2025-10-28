#include "simple_audio.hpp"
#include "log.hpp"
#include "fileio.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <SDL3/SDL.h>

// Audio decoding libraries
#define DR_WAV_IMPLEMENTATION
#include <dr_libs/dr_wav.h>
#define DR_FLAC_IMPLEMENTATION
#include <dr_libs/dr_flac.h>
#define STB_VORBIS_HEADER_ONLY
#include <stb/stb_vorbis.c>

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

	// Detect file format by extension
	std::string ext;
	size_t dot_pos = filename.find_last_of('.');
	if (dot_pos != std::string::npos)
	{
		ext = filename.substr(dot_pos + 1);
		// Convert to lowercase
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
	}

	auto audio_data = std::make_shared<AudioData>();
	
	// Decode based on format
	if (ext == "wav")
	{
		// Load WAV using dr_wav
		drwav wav;
		if (!drwav_init_memory(&wav, file_data.data(), file_data.size(), nullptr))
		{
			log::error("Failed to load WAV file: {}", filename);
			return -1;
		}

		// Setup SDL_AudioSpec
		audio_data->spec.format = (wav.bitsPerSample == 16) ? SDL_AUDIO_S16 : 
		                          (wav.bitsPerSample == 32) ? SDL_AUDIO_S32 : SDL_AUDIO_U8;
		audio_data->spec.channels = wav.channels;
		audio_data->spec.freq = wav.sampleRate;

		// Read all samples
		size_t total_samples = wav.totalPCMFrameCount * wav.channels;
		if (wav.bitsPerSample == 16)
		{
			std::vector<int16_t> samples(total_samples);
			drwav_read_pcm_frames_s16(&wav, wav.totalPCMFrameCount, samples.data());
			audio_data->buffer.resize(total_samples * sizeof(int16_t));
			memcpy(audio_data->buffer.data(), samples.data(), audio_data->buffer.size());
		}
		else if (wav.bitsPerSample == 32)
		{
			std::vector<int32_t> samples(total_samples);
			drwav_read_pcm_frames_s32(&wav, wav.totalPCMFrameCount, samples.data());
			audio_data->buffer.resize(total_samples * sizeof(int32_t));
			memcpy(audio_data->buffer.data(), samples.data(), audio_data->buffer.size());
		}
		else
		{
			std::vector<uint8_t> samples(total_samples);
			drwav_read_pcm_frames(&wav, wav.totalPCMFrameCount, samples.data());
			audio_data->buffer = samples;
		}

		drwav_uninit(&wav);
	}
	else if (ext == "flac")
	{
		// Load FLAC using dr_flac
		drflac* flac = drflac_open_memory(file_data.data(), file_data.size(), nullptr);
		if (!flac)
		{
			log::error("Failed to load FLAC file: {}", filename);
			return -1;
		}

		// Setup SDL_AudioSpec
		audio_data->spec.format = (flac->bitsPerSample == 16) ? SDL_AUDIO_S16 : SDL_AUDIO_S32;
		audio_data->spec.channels = flac->channels;
		audio_data->spec.freq = flac->sampleRate;

		// Read all samples as 16-bit
		size_t total_samples = flac->totalPCMFrameCount * flac->channels;
		std::vector<int16_t> samples(total_samples);
		drflac_read_pcm_frames_s16(flac, flac->totalPCMFrameCount, samples.data());
		
		audio_data->buffer.resize(total_samples * sizeof(int16_t));
		memcpy(audio_data->buffer.data(), samples.data(), audio_data->buffer.size());

		drflac_close(flac);
	}
	else if (ext == "ogg")
	{
		// Load OGG using stb_vorbis
		int error = 0;
		stb_vorbis* vorbis = stb_vorbis_open_memory(
			file_data.data(), 
			static_cast<int>(file_data.size()), 
			&error, 
			nullptr
		);
		
		if (!vorbis)
		{
			log::error("Failed to load OGG file: {} (error: {})", filename, error);
			return -1;
		}

		stb_vorbis_info info = stb_vorbis_get_info(vorbis);
		
		// Setup SDL_AudioSpec
		audio_data->spec.format = SDL_AUDIO_S16;
		audio_data->spec.channels = info.channels;
		audio_data->spec.freq = info.sample_rate;

		// Get total samples
		int total_frames = stb_vorbis_stream_length_in_samples(vorbis);
		size_t total_samples = total_frames * info.channels;
		
		std::vector<int16_t> samples(total_samples);
		int samples_read = stb_vorbis_get_samples_short_interleaved(
			vorbis, 
			info.channels, 
			samples.data(), 
			static_cast<int>(total_samples)
		);

		audio_data->buffer.resize(samples_read * info.channels * sizeof(int16_t));
		memcpy(audio_data->buffer.data(), samples.data(), audio_data->buffer.size());

		stb_vorbis_close(vorbis);
	}
	else
	{
		log::error("Unsupported audio format: {} (supported: WAV, FLAC, OGG)", ext);
		return -1;
	}

	internal::audio_files[hash] = audio_data;
	
	log::info("Loaded audio file: {} (ID: {}, format: {}, {}Hz, {} channels)", 
	          filename, hash, ext, audio_data->spec.freq, audio_data->spec.channels);
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
