import "xs" for Data, Input, Render    // Import the parts of the xs we will be using
import "random" for Random             // Random is a part of the Wren library

class Vec2 {
    construct new(x, y) {
        _x = x
        _y = y
    }

    x=(value) { _x = value }
    x { _x }
    y=(value) { _y = value }
    y { _y }

    - (other) {
        return Vec2.new(_x - other.x, _y - other.y)
    }

    + (other) {
        return Vec2.new(_x + other.x, _y + other.y)
    }

    * (number) {
        return Vec2.new(_x * number, _y * number)
    }

    length() {
        return (_x * _x + _y * _y).sqrt
    }

    normalize() {
        var l = length()
        _x = _x / l
        _y = _y / l
    }
}

// The game class it the entry point to your game
class Game {

    // The config method is called before the device, window, renderer
    // and most other systems are created. You can use it to change the
    // window title and size (for example).
    static config() {       
        System.print("Print config to the console")

        // This can be saved to the system.json using the
        // Data UI. This code overrides the values from the system.json
        // and can be removed if there is no need for that
        Data.setString("Title", "Sample", Data.system)
        Data.setNumber("Width", 360, Data.system)
        Data.setNumber("Height", 240, Data.system)
        Data.setNumber("Multiplier", 3, Data.system)

    }

    // The init method is called when all system have been created.
    // You can initialize you game specific data here.
    static init() {       
        System.print("Print init to the console after config")

        /* Test the Vec2 class - xs provides one too -

        var a = Vec2.new(5.0, 8.0)
        var b = Vec2.new(1.0, 3.0)
        var c = a + b
        System.print("c=[%(c.x),%(c.y)]")
        var l = c.length()
        System.print("|c|=%(l)")
        c.normalize()
        l = c.length()
        System.print("|c|=%(l)")
        */

        __player = Vec2.new(-100.0, -100.0)
        __enemy = Vec2.new(100.0, 100.0)
        __l = 0
    }        
    
    // The update method is called once per tick.
    // Gameplay code goes here.
    static update(dt) {
        var speed = 2.0

        if(Input.getKey(Input.keyLeft)) {
            __player.x = __player.x - speed
        }
        if(Input.getKey(Input.keyRight)) {
            __player.x = __player.x + speed
        }
        if(Input.getKey(Input.keyDown)) {
            __player.y = __player.y - speed
        }
        if(Input.getKey(Input.keyUp)) {
            __player.y = __player.y + speed
        }

        var d = __player - __enemy
        __l = d.length()
        d.normalize()
        d = d * (speed * 0.3)
        __enemy = __enemy + d        
    }

    static render() {
        Render.setColor(0.5, 1.0, 0.5)
        Render.disk(__player.x, __player.y, 6.0, 32)

        Render.setColor(1.0, 0.5, 0.5)
        Render.disk(__enemy.x, __enemy.y, 36.0, 32)

        if(__l < (36 - 6.0)){
            Render.disk(0.0, 0.0, 360.0, 12)
        }
    }
}