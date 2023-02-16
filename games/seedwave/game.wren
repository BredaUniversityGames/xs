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

    static config() {}    

    static init() {
        Entity.init()      
        Boss.init()
        Evolver.init()

        Background.createBackground()
        __random = Random.new()
        __player = Player.create()
        Create.playerHealthBar()

        nextBoss()
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
        __bossTime = __bossTime + dt
        var playerUnits = Entity.withTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.withTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.withTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.withTag(Tag.Computer | Tag.Bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)
        Game.collide(playerBullets, playerUnits)

        {
            var pu = __player.getComponent(Unit)
            __bossDamage = __bossDamage + pu.damage
        }


        if(__boss.deleted) {            
            Game.nextBoss()
        }

        Render.setColor(1, 1, 1, 1)
        var y = 140
        var x = -310
        Render.shapeText("DNA:%(Evolver.currentDna)", x, y, 1)
        y = y - 10
        Render.shapeText("BossTime:%(__bossTime.round)", x, y, 1)
        y = y - 10
        var pu = __player.getComponent(Unit)
        Render.shapeText("P Health:%(pu.health)", x, y, 1)
        y = y - 10
        var cu = __boss.getComponent(Unit)
        Render.shapeText("Core Health:%(cu.health)", x, y, 1)
        y = y - 10
        var bb = __boss.getComponent(Boss)
        Render.shapeText("Boss Max Health:%(bb.maxHealth)", x, y, 1)
        y = y - 10
        Render.shapeText("Boss Health:%(bb.health)", x, y, 1)
        y = y - 10
        Render.shapeText("Boss Damage:%(__bossDamage)", x, y, 1)
    }

    static nextBoss() {
        if(__healthBar != null) {
            __healthBar.delete()
            __coreBar.delete()
            Evolver.addScoreToCurrent(__bossDamage)
        }

        __bossTime = 0
        __bossDamage = 0
        
        var dna = Evolver.getNextDna()
        __boss = Boss.create(dna)
        __healthBar = Create.bossHealthBar()
        __coreBar = Create.coreHealthBar()
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
                        //var ref = Vec2.reflect(bB.velocity, d.normal) 
                        bB.velocity = bB.velocity * -0.6
                        bT.position = bT.position + (bB.velocity.normal * dis)
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

import "boss" for Boss
import "bullets" for Bullet
import "player" for Player
import "create" for Create
import "evolver" for Evolver