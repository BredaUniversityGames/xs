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

        // TODO" Move to Registry
        //Configuration.width = 640
        //Configuration.height = 360
        //Configuration.multiplier = 2
        //Configuration.title = "Seed Wave"        
    }    

    static init() {
        Registry.setNumber("a number", 2.48, Registry.game)
        Registry.setBool("a bool", false, Registry.game)
        Registry.setColor("a color", 0xFF00FFFF, Registry.game)

        //Registry.setNumber("a number", 2.48, Registry.game)
        //var num = Registry.getNumber("Print All Entities")        
        // System.print("init")
        Entity.init()
        var background = Background.createBackground()
    }    
    
    static update(dt) {
        Entity.update(dt)
        

        var aBool = Registry.getBool("a bool")
        var aNum = Registry.getNumber("a number")
        var aColor = Registry.getColor("a color")
        System.print("update aBool=%(aBool) aNum=%(aNum) aColor=%(aColor)")

        // var num = Registry.getNumber("Print All Entities")
    }

    static render() {
        Renderable.render()
        // System.print("render")
    }
}
