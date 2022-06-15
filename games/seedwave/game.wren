import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Renderable
import "background" for Background
import "create" for Create


class GameState {
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {

    static config() {
        // TODO" Move to Registry (or not)
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 2
        Configuration.title = "Seed Wave"
    }    

    static init() {
        Entity.init()        
        Registry.setBool("Print Entities", false, Registry.debug)
        __background = Background.createBackground()
        __player = Create.player()
    }    
    
    static update(dt) {
        Entity.update(dt)
    
        if(Registry.getBool("Print Entities")) {
            Entity.print()
            Registry.setBool("Print Entities", false, Registry.debug)            
        }
    }

    static render() {
        Renderable.render()
    }
}
