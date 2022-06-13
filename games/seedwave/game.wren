import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Renderable
import "background" for Background


class GameState {
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {

    static config() {
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 2
        Configuration.title = "Seed Wave"

        // TODO" Move to
        //Configuration.width = 640
        //Configuration.height = 360
        //Configuration.multiplier = 2
        //Configuration.title = "Seed Wave"        
    }    

    static init() {        
        // System.print("init")
        Entity.init()
        var background = Background.createBackground()
    }    
    
    static update(dt) {
        Entity.update(dt)
        // System.print("update")\\

        var num = Registry.getNumber("Print All Entities")
    }

    static render() {
        Renderable.render()
        // System.print("render")
    }
}
