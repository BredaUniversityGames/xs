import "xs" for Render, Data
import "xs_math" for Math, Color
import "xs_tools" for ShapeBuilder
import "background" for Background

class Game {

    static initialize() {        
        __time = 0
        __image = Render.loadImage("[game]/images/gradient.png")
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 40)
        __shape = null
        __background = Background.new()
    }    

    static update(dt) {
        __time = __time + dt
        __background.update(dt)
    }

    static render() {
        __background.render()

        var sb = ShapeBuilder.new()

        var t = (__time * 2).sin * 0.5 + 0.5
        t = t * t * t
        t = 1 - t
        
        var radius = 180 * Math.lerp(0.5, 1, t)
        var segments = 256
        var angle = 0
        var step = Math.pi * 2 / segments
        sb.addPosition(0, 0)
        sb.addTexture(1.0, 0.0)
        for(i in 0...segments) {
            var r = 1
            r = r + ((i * 6) / segments * Math.pi * 2).cos * 0.2 * t
            r = r + (0.3 + __time * 10 + (i * 24) / segments * Math.pi * 2).cos * 0.1 * t
            r = r + (0.5 + __time * -6 + (i * 12) / segments * Math.pi * 2).cos * 0.1 * t
            r = r * radius

            var x = angle.cos * r
            var y = angle.sin * r
            sb.addPosition(x, y)
            sb.addTexture(0.0, 0.0)

            angle = angle + step
        }
        for(i in 1..(segments+1)) {
            sb.addIndex(0)
            sb.addIndex(i)
            sb.addIndex(i % segments + 1)
        }

        // It can take a while for shapes to recycle,
        // so we need to destroy them manually
        if(__shape != null) {
            Render.destroyShape(__shape)
        }

        __shape = sb.build(__image)
 
        // Render the shape
        Render.sprite(
            __shape,
            0, 0, 0,        // Position
            1, 0,           // Scale аnd rotation
            0xFFFFFFFF,     // Multiply color
            0xFFFFFFFF,     // Add color
            0)              // Flags

        // Render the shape again, but smaller
        Render.sprite(
            __shape,
            0, 0, 0,        // Position
            0.75, 0,         // Scale аnd rotation
            0xFFFFFFFF,     // Multiply color
            0x00000000,     // Add color
            0)              // Flags
            
        // Render text
        var text = t > 0.2 ? "flower" : "circle"
        Render.text(__font, text,
             0, -320, 0.0,
             0xffffffff, 0x00000000, Render.spriteCenterX)   
    }
}