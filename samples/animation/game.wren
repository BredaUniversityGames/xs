// This is just confirmation, remove this line as soon as you
// start making your game
System.print("Wren just got compiled to bytecode")

// The xs module is where the inteface to the engine is
import "xs" for Render, Data
// The xs_math module where you will find the math tools and 
// a handy color class
import "xs_math" for Math, Color

// The game class it the entry point to your game
class Game {

    // The init method is called when all system have been created.
    // You can initialize you game specific data here.
    static initialize() {        
        System.print("init")

        // The "__" means that __time is a static variable (belongs to the class)
        __time = 0

        // Variable that exists only in this function 
        var image = Render.loadImage("[shared]/images/white.png")
        __sprite = Render.createSprite(image, 0, 0, 1, 1)

        // We can reuse the image variable to load another image
        // This time from the game folder
        image = Render.loadImage("[game]/images/halfdisc.png")
        __hd = Render.createSprite(image, 0, 0, 1, 1)

        // xs supports true type fonts
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 40)
    }    

    // The update method is called once per tick.
    // Gameplay code goes here.
    static update(dt) {
        __time = __time + dt
    }

    // The render method is called once per tick, right after update.
    static render() {
        // Render a gradient background
        var fromColor = Data.getColor("From Color")
        var toColor = Data.getColor("To Color")
        fromColor = Color.fromNum(fromColor)
        toColor = Color.fromNum(toColor)
        var angle = __time.sin * 0.5 + 0.5
        for(i in 0...16) {            
            var x = (i + 1) * -128 + 640
            var offset = (__time + i ).sin * 40.0
            var t = i / 16  
            var color = fromColor * (1 - t) + toColor * t
            Render.sprite(__sprite, x + offset, -360, -i, 320, Math.pi * -0.25, color.toNum, 0x00000000, 0)    
        }

        // Render half discs to form the xs logo
        // Make periodic time go from 0 to 1 and back to 0
        var t = __time * 0.5
        t = t % 2.0 < 1.0 ? t % 1.0 : 1.0 - t % 1.0
        // Smootstep the t value a few times to get snappy animation
        for(i in 0...8) t = t * t * (3 - 2 * t)
        var x = -128 - 16
        var y = -64
        Render.sprite(__hd, x, y + t * 64, 0, 1, 0.0, 0xffffffff, 0x00000000, 0)
        y = y + 64
        Render.sprite(__hd, x, y - t * 64, 0, 1, 0.0, 0xffffffff, 0x00000000, Render.spriteFlipY)
        y = y - 64
        x = x + 128
        Render.sprite(__hd, x + 32 * t, y, 0, 1, 0.0, 0xffffffff, 0x00000000, Render.spriteFlipY)
        x = x + 64
        y = y + 64
        Render.sprite(__hd, x - 32 * t, y, 0, 1, 0.0, 0xffffffff, 0x00000000, 0)

        // Render text
        Render.text(__font, "Made with xs!", 0, -120, 1, 0xffffffff, 0x00000000, Render.spriteCenterX)   
    }
}