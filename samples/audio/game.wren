import "xs" for Input, Render, Data, SimpleAudio
import "xs_math" for Math, Color

class Game {
    static initialize() {
        System.print("Audio Sample - Initializing")
        
        // Load audio files
        __beep = SimpleAudio.load("[game]/sounds/beep.wav")
        __boop = SimpleAudio.load("[game]/sounds/boop.wav")
        __click = SimpleAudio.load("[game]/sounds/click.wav")
        
        // Load visuals
        var image = Render.loadImage("[shared]/images/white.png")
        __sprite = Render.createSprite(image, 0, 0, 1, 1)
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 32)
        
        // Initialize state
        __time = 0
        __channel = -1
        __volume = 0.8
    }
    
    static update(dt) {
        __time = __time + dt
        
        // Play sounds with keyboard
        if (Input.getKeyOnce(Input.keySpace)) {
            __channel = SimpleAudio.play(__beep, __volume)
            System.print("Playing beep on channel %(__channel)")
        }
        
        if (Input.getKeyOnce(Input.keyB)) {
            __channel = SimpleAudio.play(__boop, __volume)
            System.print("Playing boop on channel %(__channel)")
        }
        
        if (Input.getKeyOnce(Input.keyC)) {
            __channel = SimpleAudio.play(__click, __volume)
            System.print("Playing click on channel %(__channel)")
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
            }
        }
        
        // Stop all sounds
        if (Input.getKeyOnce(Input.keyA)) {
            SimpleAudio.stopAll()
            System.print("Stopped all sounds")
        }
    }
    
    static render() {
        // Background gradient
        var fromColor = Color.fromHex(0x1a1a2e)
        var toColor = Color.fromHex(0x16213e)
        
        for(i in 0...16) {
            var x = (i + 1) * -128 + 640
            var t = i / 16
            var color = fromColor * (1 - t) + toColor * t
            Render.sprite(__sprite, x, -360, -i, 320, 0, color.toNum, 0x00000000, 0)
        }
        
        // Title
        Render.text(__font, "SimpleAudio Sample", 0, -280, 1, 0xeaeaeaff, 0x00000000, Render.spriteCenterX)
        
        // Instructions
        var y = -180
        var lineHeight = 45
        Render.text(__font, "Press SPACE to play beep", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__font, "Press B to play boop", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__font, "Press C to play click", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__font, "Press UP/DOWN to change volume", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__font, "Press S to stop current sound", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        y = y + lineHeight
        Render.text(__font, "Press A to stop all sounds", 0, y, 1, 0xccccccff, 0x00000000, Render.spriteCenterX)
        
        // Volume indicator
        y = y + lineHeight * 2
        var volumeText = "Volume: %((__volume * 100).floor)%%"
        Render.text(__font, volumeText, 0, y, 1, 0xffd700ff, 0x00000000, Render.spriteCenterX)
        
        // Volume bar
        y = y + lineHeight
        var barWidth = 400
        var barHeight = 30
        var barX = -barWidth / 2
        
        // Background bar
        Render.sprite(__sprite, barX, y, 0, barWidth, barHeight, 0x444444ff, 0x00000000, 0)
        
        // Volume bar
        var volumeWidth = barWidth * __volume
        Render.sprite(__sprite, barX, y, 1, volumeWidth, barHeight, 0xffd700ff, 0x00000000, 0)
        
        // Channel status
        if (__channel >= 0 && SimpleAudio.isPlaying(__channel)) {
            y = y + lineHeight * 2
            var statusText = "Playing on channel %(__channel)"
            Render.text(__font, statusText, 0, y, 1, 0x00ff00ff, 0x00000000, Render.spriteCenterX)
        }
    }
}
