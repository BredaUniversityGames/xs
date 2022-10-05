import "xs" for Input, Render, Data
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
        Data.setString("Title", "~~seedwave~~", Data.system)
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 1, Data.system)
        Data.setBool("Fullscreen", false, Data.system)
    }    

    static init() {
        Entity.init()      
        Boss.init()

        __background = Background.createBackground()
        __random = Random.new()
        __player = Player.create()

        __size = 2
        __boss = Boss.randomBoss(__size)
        __totalTime = 0
        __bossTime = 0      
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
        __totalTime = __totalTime + dt
        __bossTime = __bossTime + dt
        var playerUnits = Entity.entitiesWithTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.entitiesWithTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.entitiesWithTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.entitiesWithTag(Tag.Computer | Tag.Bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)
        Game.collide(playerBullets, playerUnits)

        if(__boss.deleted) {            
            Game.nextBoss()
        }

        Render.setColor(1, 1, 1, 1)
        Render.text("Time:%(__totalTime.round) BossTime:%(__bossTime.round) DNA:%(Boss.dna)", -300, 150, 1)

        //Render.text("DNA:%(Boss.dna)", -50, 100, 1)
    }

    static nextBoss() {
        __bossTime = 0
        __size = __size + 1
        __boss = Boss.randomBoss(__size)
    }

    static collide(bullets, units) {        
        for(u in units) {                       
            for(b in bullets) {     
                var uT = u.getComponent(Transform)
                var bT = b.getComponent(Transform)
                var uB = u.getComponent(Body)
                var bB = b.getComponent(Body)
                var d = uT.position - bT.position
                var dis = d.magnitude
                if(dis < (uB.size + bB.size)) {                    
                    var un = u.getComponent(Unit)
                    var bl = b.getComponentSuper(Bullet)
                    un.damage(bl.damage)
                    if(Bits.checkBitFlag(u.tag, Tag.Deflect)) {
                        //var ref = Vec2.reflect(bB.velocity, d.normalise) 
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

import "boss" for Boss
import "bullets" for Bullet
import "player" for Player
