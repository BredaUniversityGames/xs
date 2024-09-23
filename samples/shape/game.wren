import "xs" for Render, Data, Vector
import "xs_math" for Math, Color
import "xs_tools" for ShapeBuilder

class Game {
    static config() {}

    static init() {        
        __time = 0
        __image = Render.loadImage("[shared]/images/white.png")
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 40)
        __shape = null
    }    

    static update(dt) { __time = __time + dt }

    static render() {
        var sb = ShapeBuilder.new()

        // Vertex 0
        sb.addPosition(0, 0)
        sb.addTexture(0, 0)

        // Vertex 1        
        sb.addPosition(10, 0)
        sb.addTexture(1, 0)
        
        // Vertex 2
        sb.addPosition(10, 10)
        sb.addTexture(1, 1)
        
        // Vertex 3
        sb.addPosition(0, 10)
        sb.addTexture(0, 1)
        
        // Indices
        sb.addIndex(0)
        sb.addIndex(1)
        sb.addIndex(2)
        sb.addIndex(2)
        sb.addIndex(3)
        sb.addIndex(0)

        if(__shape != null) {
            Render.destroyShape(__shape)
        }

        __shape = sb.build(__image)
 
        
        Render.sprite(
            __shape,
            0, 0, 0,        // Position
            1, 0,           // Scale аnd rotation
            0xFFFFFFFF,     // Multiply color
            0x00000000,     // Add color
            0)              // Flags
    }
}