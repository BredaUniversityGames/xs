import "xs" for Configuration, Input, Render    // These are the parts of the xs we will be using
import "random" for Random                      // Random is a part of the Wren library

class Game {

    static init() {       
        // init gets called by our engine once, when the scipt is initialilzed

        // System.print is a built in function in the engine.
        // It can print text to the console 
        System.print("Init")

        // This is how you configure the window in xs
        Configuration.width = 360
        Configuration.height = 240
        Configuration.title = "Window Title"
    }        
    
    static update(dt) {
        // udptate gets called by our engine every frame
        // dt stands for "delta time" and it's the time from the previous frame

        // System.print can also print values of variables
        System.print("Update dt=%(dt)")
    }
}
