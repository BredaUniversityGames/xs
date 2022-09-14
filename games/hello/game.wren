// This is just confrimation, remove this line as soon as you
// start making your game
System.print("Wren just got compiled to bytecode")

// The xs module is 
import "xs" for Render, Data

// The game class it the entry point to your game
class Game {

    // The config methos is called before the device, window, renderer
    // and most other systems are created. You can use it to chage the
    // window title and size (for example).
    // You can remove this method
    static config() {
        System.print("config")
    }

    // The init method is called when all system have been created.
    // You can initilize you game specific data here.
    static init() {        
        System.print("init")

        // The "__" means that __time is a static variable (belongs to the class)
        __time = 0
    }    
    
    // The uddate method is called once per tick.
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
        Render.text("xs", -100, 100, 20)
        Render.text("Made with love at Games@BUas", -100, -50, 1)
        Render.setColor(0.5, 0.5, 0.5)
        Render.text("Time: %(__time)", -300, -160, 1)
    }
}
