import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Renderable, Body, Transform

class GameState {
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {

    static config() { /* loaded from system.json */ }    

    static init() {
        Entity.init()
        Create.init()
        
        __background = Background.createBackground()
        __random = Random.new()
        __player = Player.create()
        __size = 2

        Create.Enemy(Vec2.new(-100, 250), Size.S, 0)
        Create.Enemy(Vec2.new(0, 250), Size.S, 1)
        Create.Enemy(Vec2.new(100,250), Size.S, 2)

        Create.Enemy(Vec2.new(-200,   100), Size.M, 0)
        Create.Enemy(Vec2.new(-100, 100), Size.M, 1)
        Create.Enemy(Vec2.new(0, 100), Size.M, 2)
        Create.Enemy(Vec2.new(100, 100), Size.M, 3)
        Create.Enemy(Vec2.new(200, 100), Size.M, 4)

        Create.Enemy(Vec2.new(-150,   -150), Size.L, 2)
        Create.Enemy(Vec2.new(0, -150), Size.L, 1)
        Create.Enemy(Vec2.new(150, -150), Size.L, 0)

        __totalTime = 0
        __bossTime = 0      
    }    
    
    static update(dt) {
        Entity.update(dt)
        // Game.updateGame(dt)
        
        if(Data.getBool("Print Entities", Data.debug)) {
            Entity.print()
            Data.setBool("Print Entities", false, Data.debug)            
        }

        if(Data.getBool("Debug Render", Data.debug)) {
            debugRender()
        }
    }

    static updateGame(dt) {
        return
        __totalTime = __totalTime + dt
        __bossTime = __bossTime + dt
        var playerUnits = Entity.withTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.withTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.withTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.withTag(Tag.Computer | Tag.Bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)
        Game.collide(playerBullets, playerUnits)

        Render.setColor(1, 1, 1, 1)
        // Render.shapeText("Time:%(__totalTime.round) BossTime:%(__bossTime.round) DNA:%(Boss.dna)", -300, 150, 1)
    }

    static nextBoss() {
        __bossTime = 0
        __size = __size + 1
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
        Renderable.render()
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

import "bullets" for Bullet
import "player" for Player
import "background" for Background
import "debug" for DebugColor
import "tags" for Tag, Team, Size
import "unit" for Unit
import "random" for Random
import "create" for Create
