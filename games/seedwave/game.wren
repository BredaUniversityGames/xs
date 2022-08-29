import "xs" for Configuration, Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
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
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 2
        Configuration.title = "Seed Wave"
    }    

    static init() {
        Entity.init()      
        Boss.init()

        __background = Background.createBackground()
        __player = Create.player()
        __boss = Boss.randomBoss(20)
        //  __boss = Create.boss("C1SL2C2M1L1SL2C1L2C1")
        //__boss = Create.boss("C1L2C2M1L1L2M1C1L2C1L2C2M1L1L2M1C1L2")
    }    
    
    static update(dt) {
        Entity.update(dt)
    
        if(Data.getBool("Print Entities", Data.debug)) {
            Entity.print()
            Data.setBool("Print Entities", false, Data.debug)            
        }

        if(Data.getBool("Debug Render", Data.debug)) {
            debugRender()
        }
    }

    static render() {
        if(Data.getBool("Renderable Render", Data.debug)) {
            Renderable.render()
        }
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
import "boss" for Boss
