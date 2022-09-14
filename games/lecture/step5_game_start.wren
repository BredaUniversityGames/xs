import "xs" for Input, Render    // These are the parts of the xs we will be using
import "random" for Random                      // Random is a part of the Wren library

class Game {

    static init() {       
        // init gets called by our engine once, when the scipt is initialilzed

        // This is how you configure the window in xs
        Data.getNumber("Width", Data.system) = 360
        Data.getNumber("Height", Data.system) = 240
        Configuration.title = "Game"

        // In Wren, varaibles starting with __ belong to the class. In this case
        // the Game class. They are equivalent to globals/static in other languages 
        __x = 0.0
        __y = 0.0
    }        
    
    static update(dt) {
        // udptate gets called by our engine every frame
        // dt stands for "delta time" and it's the time from the previous frame

        var speed = 2.0         // How fast will the player move

        if(Input.getKey(Input.keyLeft)) {
            __x = __x - speed
        }
        if(Input.getKey(Input.keyRight)) {
            __x = __x + speed
        }
        if(Input.getKey(Input.keyDown)) {
            __y = __y - speed
        }
        if(Input.getKey(Input.keyUp)) {
            __y = __y + speed
        }
                
        Render.setColor(0.5, 1.0, 0.5)
        Render.disk(__x, __y, 5.0, 32)
    }
}
