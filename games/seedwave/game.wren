import "xs" for Configuration, Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Renderable, Body, Transform
import "background" for Background
import "debug" for DebugColor
import "tags" for Tag, Team
import "unit" for Unit
import "random" for Random


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
        __player = Player.create()
        __boss = Boss.randomBoss(3)
        __random = Random.new()
    }    
    
    static update(dt) {
        Entity.update(dt)

        Game.updateGame(dt)
    
        if(Data.getBool("Print Entities", Data.debug)) {
            Entity.print()
            Data.setBool("Print Entities", false, Data.debug)            
        }

        if(Data.getBool("Debug Render", Data.debug)) {
            debugRender()
        }
    }

    static updateGame(dt) {
        var playerUnits = Entity.entitiesWithTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.entitiesWithTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.entitiesWithTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.entitiesWithTag(Tag.Computer | Tag.Bullet)

        //Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)
    }

    static collide(bullets, units) {        
        for(u in units) {                       
            for(b in bullets) {     
                var uT = u.getComponent(Transform)
                var bT = b.getComponent(Transform)
                var uB = u.getComponent(Body)
                var bB = b.getComponent(Body)
                var dis = uT.position - bT.position
                dis = dis.magnitude
                if(dis < (uB.size + bB.size)) {                    
                    var un = u.getComponent(Unit)
                    var bl = b.getComponentSuper(Bullet)
                    un.damage(bl.damage)
                    if(Bits.checkBitFlag(u.tag, Tag.Deflect)) {
                        bB.velocity = bB.velocity * -0.6
                        bT.position = bT.position + (bB.velocity.normalise * dis)
                    } else {
                        b.delete()
                    }                    
                    // Render.disk(uT.position.x, uT.position.y, 2, 24)
                }
            }
        }
    }

    static render() {
        if(Data.getBool("Renderable Render", Data.debug)) {
            Renderable.render()
        }
    }

    static player { __player }
    static random { __random }

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
import "bullets" for Bullet
import "player" for Player
