import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Renderable, Body, Transform
import "random" for Random

class GameState {
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {

    static config() { }    

    static init() {
        __random = Random.new()
        Entity.init()      

        Create.background()
        __player = Create.player()        
        __enemy = Create.enemy()

        __size = 3
        // __boss = Boss.randomBoss(__size)
        // System.print(Game.boss.getComponent(Boss))
        // __healthBar = Create.bossHealthBar()
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
        var playerUnits = Entity.withTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.withTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.withTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.withTag(Tag.Computer | Tag.Bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)
        // Game.collide(playerBullets, playerUnits)
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
                    var bl = b.getComponentSuper(Damage)
                    System.print(bl)
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
    static boss { __boss }
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
import "create" for Create
import "damage" for Damage
import "debug" for DebugColor
import "tags" for Tag, Team
import "unit" for Unit
