# Audio Sample

This sample demonstrates the SimpleAudio API for playing sounds using SDL3 with support for multiple audio formats.

## Features

- Load audio files (WAV, OGG, FLAC formats)
- Play sounds with volume control
- Stop individual or all sounds
- Check playback status
- Compare compression efficiency (WAV vs OGG vs FLAC)

## Supported Formats

- **WAV** - Uncompressed PCM audio (large file size, best quality)
- **OGG Vorbis** - Lossy compression (smallest file size, good quality)
- **FLAC** - Lossless compression (medium file size, perfect quality)

## Controls

### Short Sounds (WAV format)
- **SPACE** - Play beep sound
- **B** - Play boop sound
- **C** - Play click sound

### Music (10-second melody, test compression)
- **W** - Play WAV version (862KB, uncompressed)
- **O** - Play OGG version (24KB, lossy compressed)
- **F** - Play FLAC version (186KB, lossless compressed)

### Playback Controls
- **UP/DOWN** - Adjust volume
- **S** - Stop current sound
- **A** - Stop all sounds

## API Usage

```wren
import "xs" for SimpleAudio

// Load audio files (auto-detects format by extension)
var soundWav = SimpleAudio.load("[game]/sounds/beep.wav")
var soundOgg = SimpleAudio.load("[game]/sounds/music.ogg")
var soundFlac = SimpleAudio.load("[game]/sounds/music.flac")

// Play with default volume (1.0)
var channel = SimpleAudio.play(soundWav)

// Play with custom volume (0.0 to 1.0)
var channel = SimpleAudio.play(soundOgg, 0.5)

// Adjust volume of playing sound
SimpleAudio.setVolume(channel, 0.8)

// Get current volume
var volume = SimpleAudio.getVolume(channel)

// Check if sound is playing
if (SimpleAudio.isPlaying(channel)) {
    System.print("Sound is playing")
}

// Stop a specific sound
SimpleAudio.stop(channel)

// Stop all sounds
SimpleAudio.stopAll()
```

## Technical Details

The SimpleAudio API uses SDL3's audio subsystem for cross-platform audio playback with support for:
- **dr_wav** for WAV decoding
- **dr_flac** for FLAC decoding  
- **stb_vorbis** for OGG Vorbis decoding

All formats are decoded to PCM and streamed through SDL3's audio system for seamless playback.

## Compression Comparison

The music file demonstrates compression efficiency:
- WAV: 862 KB (uncompressed baseline)
- FLAC: 186 KB (21.6% of original, lossless)
- OGG: 24 KB (2.8% of original, lossy quality 6/10)

FLAC provides excellent compression while maintaining perfect audio quality, making it ideal for music and sound effects where file size matters. OGG Vorbis offers even better compression with imperceptible quality loss at higher quality settings.
