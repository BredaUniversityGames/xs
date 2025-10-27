# Audio Sample

This sample demonstrates the SimpleAudio API for playing sounds using SDL3.

## Features

- Load audio files (WAV format)
- Play sounds with volume control
- Stop individual or all sounds
- Check playback status

## Controls

- **SPACE** - Play beep sound
- **B** - Play boop sound
- **C** - Play click sound
- **UP/DOWN** - Adjust volume
- **S** - Stop current sound
- **A** - Stop all sounds

## API Usage

```wren
import "xs" for SimpleAudio

// Load an audio file
var sound = SimpleAudio.load("[game]/sounds/beep.wav")

// Play with default volume (1.0)
var channel = SimpleAudio.play(sound)

// Play with custom volume (0.0 to 1.0)
var channel = SimpleAudio.play(sound, 0.5)

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

The SimpleAudio API uses SDL3's audio subsystem for cross-platform audio playback. It supports WAV files natively and provides simple controls for basic audio needs.
