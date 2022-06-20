import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Renderable, Body, Transform
import "background" for Background
import "debug" for DebugColor

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
        Configuration.multiplier = 1
        Configuration.title = "Seed Wave"
    }    

    static init() {
        Entity.init()      
        Create.init()  
        Registry.setBool("Print Entities", false, Registry.debug)
        //Registry.setBool("Debug Render", false, Registry.debug)

        __background = Background.createBackground()
        __player = Create.player()
        __boss = Create.randomBoss(9)
        //  __boss = Create.boss("C1SL2C2M1L1SL2C1L2C1")
        //__boss = Create.boss("C1L2C2M1L1L2M1C1L2C1L2C2M1L1L2M1C1L2")
    }    
    
    static update(dt) {
        Entity.update(dt)
    
        if(Registry.getBool("Print Entities")) {
            Entity.print()
            Registry.setBool("Print Entities", false, Registry.debug)            
        }

        if(Registry.getBool("Debug Render")) {
            debugRender()
        }
    }

    static render() {
        Renderable.render()
    }

    static player { __player }

    static debugRender() {
        for(e in Entity.entities) {
            var b = e.getComponent(Body)
            if(b != null) {
                var t = e.getComponent(Transform)
                var c = e.getComponent(DebugColor)
                if(c != null) {
                    Render.setColor(c.color)
                } else {
                    Render.setColor(1.0, 1.0, 1.0)
                }
                Render.disk(t.position.x, t.position.y, b.size, 24)
            }
        }
    }
}

import "create" for Create
