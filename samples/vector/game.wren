import "xs" for Render, Data
import "xs_math" for Math, Color
import "xs_tools" for ShapeBuilder
import "background" for Background

class Game {

    static initialize() {        
        __time = 0
        __image = Render.loadImage("[game]/images/gradient.png")
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 40)
        __testShape = Render.loadShape("[game]/svg/test.svg")        
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

        var radius = 20
        var segments = 64
        var angle = 0
        var step = Math.pi * 2 / segments
        sb.addPosition(0, 0)
        sb.addTexture(1.0, 0.0)
        for(i in 0...segments) {
            var r = radius            
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
 

        Render.shape(
            __testShape,
            0, 0, 0,        // Position
            1, 0,           // Scale and rotation
            0xFFFFFFFF, // Multiply color
            0x00000000) // Add color
        
        Render.shape(
            __shape,
            0, 0, -1,        // Position
            1, 0,           // Scale and rotation
            0xFFFFFFFF, // Multiply color
            0xFFFFFFFF) // Add color        
    }
}