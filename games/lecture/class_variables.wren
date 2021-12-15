import "xs" for Configuration, Input, Render    // These are the parts of the xs we will be using
import "random" for Random                      // Random is a part of the Wren library

class Game {

    static init() {       
        // init gets called by our engine once, when the scipt is initialilzed

        // System.print is a built in function in the engine.
        // It can print text to the console 
        // We will turn this off for now
        // System.print("Init")

        // This is how you configure the window in xs
        Configuration.width = 360
        Configuration.height = 240
        Configuration.title = "Game"

        // In Wren, varaibles starting with __ belong to the class. In this case
        // the Game class. They are equivalent to globals/static in other languages 
        __time = 0.0
    }        
    
    static update(dt) {
        // udptate gets called by our engine every frame
        // dt stands for "delta time" and it's the time from the previous frame

        var speed = 1.0         // How fast
        var amplitude = 120     // How wide

        // We can modify any variable (even static ones) use the value
        // that was already in it
        __time = __time + dt * speed
        // ^new   ^old    ^function parameter

        // We can use this to assign a value to var x
        var x = __time.sin * amplitude
        //             ^the sin funtion of the time
        // ^local varaible in the function

        Render.setColor(1.0, 1.0, 1.0)
        Render.disk(x, 0, 10.0, 32)

        // System.print can also print values of variables
        // We will turn this off for now
        // System.print("Update dt=%(dt)")
    }
}
