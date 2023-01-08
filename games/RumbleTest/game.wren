import "xs" for Render, Data, Input

class Game {

    static config() {
        System.print("config")

        Data.setString("Title", "xs - RumbleTest", Data.system)
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 1, Data.system)
        Data.setBool("Fullscreen", false, Data.system)
    }

    static init() {        
        __time = 0
    }    

    static update(dt) {
        __time = __time + dt

        if (Input.getAxis(5)) {
            Input.setPadVibration(10,10)
            Input.setPadLightbarColor(255,20,147) //deep pink sicne thats not commonly used and clearly a change in colour
        }
    }

    static render() {
        Render.setColor(
            (__time * 10 + 1).sin.abs,
            (__time * 10 + 2).sin.abs,
            (__time * 10 + 3).sin.abs)
        Render.shapeText("RumbleTest", -150, 0, 5)
    }
}