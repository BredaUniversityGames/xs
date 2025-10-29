import "xs" for Input, Render, Data, SimpleAudio
import "xs_math" for Math, Color

class Game {
    static initialize() {
        System.print("Audio Sample - Initializing")
        
        // Load audio files in different formats
        __beep = SimpleAudio.load("[game]/sounds/beep.wav")
        __boop = SimpleAudio.load("[game]/sounds/boop.wav")
        __click = SimpleAudio.load("[game]/sounds/click.wav")
        
        // Load music in different compressed formats
        __musicWav = SimpleAudio.load("[game]/sounds/music.wav")
        __musicOgg = SimpleAudio.load("[game]/sounds/music.ogg")
        __musicFlac = SimpleAudio.load("[game]/sounds/music.flac")
        
        // Load visuals
        var image = Render.loadImage("[shared]/images/white.png")
        __sprite = Render.createSprite(image, 0, 0, 1, 1)
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 32)
        __smallFont = Render.loadFont("[shared]/fonts/selawk.ttf", 24)
        
        // Initialize state
        __time = 0
        __channel = -1
        __volume = 0.8
        __currentFormat = "none"
    }
    
    static update(dt) {
        __time = __time + dt
        
        // Play sounds with keyboard
        if (Input.getKeyOnce(Input.keySpace)) {
            __channel = SimpleAudio.play(__beep, __volume)
            System.print("Playing beep on channel %(__channel)")
            __currentFormat = "WAV"
        }
        
        if (Input.getKeyOnce(Input.keyB)) {
            __channel = SimpleAudio.play(__boop, __volume)
            System.print("Playing boop on channel %(__channel)")
            __currentFormat = "WAV"
        }
        
        if (Input.getKeyOnce(Input.keyC)) {
            __channel = SimpleAudio.play(__click, __volume)
            System.print("Playing click on channel %(__channel)")
            __currentFormat = "WAV"
        }
        
        // Music playback in different formats
        if (Input.getKeyOnce(Input.keyW)) {
            __channel = SimpleAudio.play(__musicWav, __volume)
            System.print("Playing music.WAV on channel %(__channel)")
            __currentFormat = "WAV"
        }
        
        if (Input.getKeyOnce(Input.keyO)) {
            __channel = SimpleAudio.play(__musicOgg, __volume)
            System.print("Playing music.OGG on channel %(__channel)")
            __currentFormat = "OGG"
        }
        
        if (Input.getKeyOnce(Input.keyF)) {
            __channel = SimpleAudio.play(__musicFlac, __volume)
            System.print("Playing music.FLAC on channel %(__channel)")
            __currentFormat = "FLAC"
        }
        
        // Volume control
        if (Input.getKey(Input.keyUp)) {
            __volume = (__volume + dt).min(1.0)
            if (__channel >= 0) {
                SimpleAudio.setVolume(__channel, __volume)
            }
        }
        
        if (Input.getKey(Input.keyDown)) {
            __volume = (__volume - dt).max(0.0)
            if (__channel >= 0) {
                SimpleAudio.setVolume(__channel, __volume)
            }
        }
        
        // Stop sound
        if (Input.getKeyOnce(Input.keyS)) {
            if (__channel >= 0) {
                SimpleAudio.stop(__channel)
                System.print("Stopped channel %(__channel)")
                __currentFormat = "none"
            }
        }
        
        // Stop all sounds
        if (Input.getKeyOnce(Input.keyA)) {
            SimpleAudio.stopAll()
            System.print("Stopped all sounds")
            __currentFormat = "none"
        }
    }
    
    static render() {
        // Background gradient
        var bgColor = Data.getColor("Background Color")

        // Large sprite for the whole screen

        // (spriteId, x, y, z, scale, rotation, mul, add, flags)
        Render.sprite(__sprite, 0, 0, 0, 500, 0, bgColor, 0x00000000, Render.spriteCenter)
        
        // Title
        Render.text(__font, "SimpleAudio Sample - Multi-Format", 0, -300, 1, 0xeaeaeaff, 0x00000000, Render.spriteCenterX)
        
        // Instructions
        var y = -220
        var lineHeight = 38
        Render.text(__smallFont, "Short Sounds (WAV):", 0, y, 1, 0xffd700ff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__smallFont, "SPACE: beep  |  B: boop  |  C: click", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        
        y = y + lineHeight + 10
        Render.text(__smallFont, "Music (10s melody, test compression):", 0, y, 1, 0xffd700ff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__smallFont, "W: WAV (862KB)  |  O: OGG (24KB)  |  F: FLAC (186KB)", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        
        y = y + lineHeight + 10
        Render.text(__smallFont, "Controls:", 0, y, 1, 0xffd700ff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__smallFont, "UP/DOWN: volume  |  S: stop  |  A: stop all", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        
        // Volume indicator
        y = y + lineHeight * 1.5
        var volumeText = "Volume:" + ((__volume * 100).floor).toString
        Render.text(__smallFont, volumeText, 0, y, 1, 0xffd700ff, 0x00000000, Render.spriteCenterX)
        
        // Status indicators
        y = y + lineHeight * 1.5
        if (__channel >= 0 && SimpleAudio.isPlaying(__channel)) {
            var statusText = "Playing (%(__currentFormat))"
            Render.text(__smallFont, statusText, 0, y, 1, 0x00ff00ff, 0x00000000, Render.spriteCenterX)
        } else {
            Render.text(__smallFont, "Idle", 0, y, 1, 0x888888ff, 0x00000000, Render.spriteCenterX)
        }
    }
}
