import "xs" for Configuration, Input, Render    // These are the parts of the xs we will be using
import "random" for Random                      // Random is a part of the Wren library

class Game {

    static init() {       
        // init gets called by our engine once, when the scipt is initialilzed

        // This is how you configure the window in xs
        Configuration.width = 360
        Configuration.height = 240
        Configuration.title = "Game"

        // In Wren, varaibles starting with __ belong to the class. In this case
        // the Game class. They are equivalent to globals/static in other languages 
        __x = 0.0
        __y = 0.0

        __enemyX = 100.0
        __enemyY = 100.0
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

        var dx = __x - __enemyX
        var dy = __y - __enemyY

        // Normalize
        var l = (dx * dx + dy * dy).sqrt    // Get length of the vector [dx,dy]
        dx = dx / l                         // Normalize dx
        dy = dy / l                         // Normalize dy

        speed = speed * 0.3
        __enemyX = __enemyX + dx * speed
        __enemyY = __enemyY + dy * speed
                
        Render.setColor(0.5, 1.0, 0.5)
        Render.disk(__x, __y, 6.0, 32)

        Render.setColor(1.0, 0.5, 0.5)
        Render.disk(__enemyX, __enemyY, 36.0, 32)

        if(l < (6.0 + 26)){
            Render.disk(0.0, 0.0, 360.0, 12)
        }
    }
}
