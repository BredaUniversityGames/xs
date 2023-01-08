// This is just confirmation, remove this line as soon as you
// start making your game
System.print("Wren just got compiled to bytecode")

// The xs module is 
import "xs" for Render, Data

// The game class it the entry point to your game
class Game {

    // The config method is called before the device, window, renderer
    // and most other systems are created. You can use it to change the
    // window title and size (for example).
    // You can remove this method
    static config() {
        System.print("config")
        
        // This can be saved to the system.json using the
        // Data UI. This code overrides the values from the system.json
        // and can be removed if there is no need for that
        Data.setString("Title", "xs - hello", Data.system)
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 1, Data.system)
        Data.setBool("Fullscreen", false, Data.system)
    }

    // The init method is called when all system have been created.
    // You can initialize you game specific data here.
    static init() {        
        System.print("init")

        // The "__" means that __time is a static variable (belongs to the class)
        __time = 0

        // Variable that exists only in this function 
        var image = Render.loadImage("[games]/shared/images/FMOD_White.png")
        __sprite = Render.createSprite(image, 0, 0, 1, 1)
    }    

    // The update method is called once per tick.
    // Gameplay code goes here.
    static update(dt) {
        __time = __time + dt
    }

    // The render method is called once per tick, right after update.
    static render() {
        Render.setColor(
            (__time * 10 + 1).sin.abs,
            (__time * 10 + 2).sin.abs,
            (__time * 10 + 3).sin.abs)
        Render.shapeText("xs", -100, 100, 20)
        Render.shapeText("Made with love at Games@BUas", -100, -50, 1)
        Render.setColor(0.5, 0.5, 0.5)
        Render.shapeText("Time: %(__time)", -300, -160, 1)

        Render.sprite(__sprite, 180, -152, 0, 0.16, 0.0, 0xFFFFFFFF, 0x00000000, 0)
    }
}