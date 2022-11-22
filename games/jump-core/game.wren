import "xs" for Input, Render, Data, File, Audio
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation, Label
import "random" for Random
import "globals" for Globals
import "tags" for Team, Tag
import "debug" for DebugColor
import "background" for Background

class BulletType {
    static straight     { 1 }
    static spread       { 2 }
    static directed     { 3 }
    static follow       { 4 }
}

class Bullet is Component {
    construct new(team, damage) {
        super()
        _team = team
        _damage = damage
    }

    update(dt) {
        var t = owner.getComponent(Transform)

        var w = Data.getNumber("Width", Data.system) * 0.5
        if (t.position.x < -w) {
            owner.delete()
        } else if (t.position.x > w) {
            owner.delete()
        }

        var h = Data.getNumber("Height", Data.system) * 0.5
        if (t.position.y < -h) {
            owner.delete()
        } else if (t.position.y > h) {
            owner.delete()
        }
    }

    damage { _damage }
    team { _team }

    toString { "[Bullet team:%(_team) damage:%(_damage)]" }
}

class Explosion is Component {
    construct new(duration) {
        _time = 0
        Game.addShake()
    }

    update(dt) {
        _time = _time + dt
        var b = owner.getComponent(Body)
        b.size =  _time.pow(06) * 15.0
        if(_time > 1) {
            owner.delete()
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// Game
///////////////////////////////////////////////////////////////////////////////

class GameState {
    static Title    { 0 }
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {
    static config() {        
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 1, Data.system)
        Data.setString("Title", "JumpCore", Data.system)        
    }

    static init() {        
        Entity.init()
        Background.create()
        createTitle()

        __frame = 0
        __random = Random.new()
        __state = GameState.Title        
        __score = 0
        __waveTimer = 0
        __wave = 0
        __shakeOffset = Vec2.new(0, 0)
        __shakeIntesity = 0
        __time = 0.0
        __core = 0.0
        __font = Render.loadFont("[games]/jump-core/fonts/FutilePro.ttf", 18)
        __levels = ["daytime", "night", "abandoned", "snow-rain", "sunset"]

        var song = Audio.load("[game]/Blast_2019.flac", Audio.groupMusic)
        var sound = Audio.play(song)
    }        
    
    static update(dt) {        
        Entity.update(dt)
        if(__state == GameState.Title) {
            updateTitle(dt)
        } else  if(__state == GameState.Menu) {
            // __menu.update(dt)
        } else if(__state == GameState.Play) {
            updatePlay(dt)
        } else if(__state == GameState.Score) {
        } 
        
        __shakeOffset.x = __random.float(-1.0, 1.0)
        __shakeOffset.y = __random.float(-1.0, 1.0)
        __shakeIntesity = Math.damp(__shakeIntesity, 0, dt, 10)
    }

    static render() {
        Render.setOffset(__shakeOffset.x * __shakeIntesity, __shakeOffset.y * __shakeIntesity)
        Renderable.render()        

        if(__state == GameState.Play) {
            Render.setOffset(0, 0)
            var pu = playerShip.getComponent(Unit)

            var bars = "|"
            for(i in 0...(__core * 10)) {
                bars = bars + "|"
            }

            var text = "CORE %(bars)   :    SCORE %(__score)   :  WAVE %(__wave)  :  HEALTH %(pu.health)"
            Render.text(__font, text, 0, 150, 0xFFFFFFFF, 0x00000000, Render.spriteCenter)
        }
    }

    static startPlay() {
        __score = 0
        __waveTimer = 0
        __wave = 0
        __state = GameState.Play
        __ship = Player.createShip()
        __menu.delete()
        __menu = null
        __core = 0.0

        Portal.create()
    }

    static jump() {
        __core = 0.0
        var i = __random.int(0, __levels.count)
        var level = __levels[i]
        __ship.getComponent(Player).setLevel(level)
        Background.setLevel(level)
    }

    static updateTitle(dt) {
        if (Input.getButtonOnce(0) == true || Input.getKeyOnce(Input.keySpace)) {
            startMenu()
        }
    }

    static updatePlay(dt) {
        var pu = playerShip.getComponent(Unit)
        __time = __time + dt

        __core = __core + dt * 0.1

        if(Data.getBool("Show Physics", Data.debug)) {
            for(e in Entity.entities) {
                var b = e.getComponent(Body)
                if(b != null) {
                    var t = e.getComponent(Transform)
                    var c = e.getComponent(DebugColor)
                    Render.setColor(1.0, 0.0, 0.0)
                    if(c != null) {
                        Render.setColor(c.color)
                    }
                    Render.disk(t.position.x, t.position.y, b.size, 24)
                }
            }
        }
 
        var playerUnits = Entity.withTag(Tag.player | Tag.unit)
        var playerBullets = Entity.withTag(Tag.player | Tag.bullet)
        var computerUnits = Entity.withTag(Tag.computer | Tag.unit)
        var computerBullets = Entity.withTag(Tag.computer | Tag.bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)

        if(computerUnits.count == 0) {
            __waveTimer = __waveTimer + dt            
        }

        if(__waveTimer >= 1.5) {
            for(w in 0..__wave) {
                createEnemyShips()
            }
            __waveTimer= 0
            __wave = __wave + 1
        }

        if(playerShip.getComponent(Unit).health <= 0) {
            endGame()            
        }
    }

    static startMenu() {
        __title.delete()
        __title = null
        __state = GameState.Menu    
        __menu = MainMenu.create()

    }

    static endGame() {        
        __state = GameState.Score
        __score = ScoreMenu.create()
    }

    static addShake() {
        __shakeIntesity = 4.0
    }

    static createTitle() {
        __title = Entity.new()
        {
            var t = Transform.new(Vec2.new(0,0))
            __title.addComponent(t)
        }
        { // Text part
            var e = Entity.new()
            var t = Transform.new(Vec2.new(0,0))
            var r = Relation.new(__title)
            var s = Sprite.new("[games]/jump-core/images/backgrounds/title.png", 0, 0, 1, 1)
            s.layer = 10.0
            s.flags = Render.spriteCenter            
            e.name = "Title"
            //bullet.tag = Tag.player | Tag.bullet
            e.addComponent(t)
            e.addComponent(s)
            e.addComponent(r)
        }
        { // Core part
            var e = Entity.new()
            var t = Transform.new(Vec2.new(65,0))
            var r = Relation.new(__title)
            r.offset = Vec2.new(58, 10)
            var s = AnimatedSprite.new("[games]/jump-core/images/vfx/Electric_Effect_05.png", 4, 4, 15)
            s.layer = 10.1
            s.flags = Render.spriteCenter
            s.addAnimation("play", [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14])
            s.playAnimation("play")
            e.name = "Core"
            //bullet.tag = Tag.player | Tag.bullet
            e.addComponent(t)
            e.addComponent(s)
            e.addComponent(r)
        }
        { // Any button
            var e = Entity.new()
            var t = Transform.new(Vec2.new(0,0))
            var r = Relation.new(__title)
            r.offset = Vec2.new(0, -29)
            var l = Label.new("[games]/jump-core/fonts/FutilePro.ttf", "Press any button to start", 18)
            l.layer = 11.0
            l.flags = Render.spriteCenter                                    
            e.name = "Any Button"
            e.addComponent(t)
            e.addComponent(l)
            e.addComponent(r)
        }
    }

    static createEnemyShips() {
        var x = Game.random.float(0, 200)
        var y = Game.random.float(-100, 100)
        var pos = Vec2.new(x, y)
        var tilt = Game.random.float(0, 5.0)

        var bulletType = Game.random.int(1, 4)
        var core = createEnemyCore(pos, tilt)
        var orbitor = core.getComponent(Orbitor)
        for(i in 0..6) {
            var ship = createEnemyShip(i, tilt, core, bulletType)
            orbitor.add(ship)
        }
    }

    static createEnemyCore(pos, tilt) {
        var core = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p) 
        var v = Vec2.new(0, 0)
        var b = Body.new(Globals.EnemyCoreSize, v)
        var e = EnemyCore.new(pos, tilt)
        var u = Unit.new(Team.computer, Globals.EnemyCoreHealth)
        var c = DebugColor.new(Globals.EnemyColor)
        var o = Orbitor.new(core)
        var s = GridSprite.new("[games]/jump-core/images/ships/purple-ship-spritesheet.png", 5, 1)
        s.layer = 0.9
        s.flags = Render.spriteCenter
        s.idx = 4
        core.addComponent(t)
        core.addComponent(b)
        core.addComponent(e)
        core.addComponent(u)
        core.addComponent(c)
        core.addComponent(o)
        core.addComponent(s)
        core.name = "Enemy Core"
        core.tag = (Tag.computer | Tag.unit)
        return core
    }

    static createEnemyShip(idx, tilt, core, bulletType) {
        var ship = Entity.new()
        var p = Vec2.new(240, 0)
        var t = Transform.new(p)    
        var v = Vec2.new(0, 0)
        var b = Body.new(Globals.EnemySize, v)
        var e = Enemy.new(idx, tilt, core, bulletType)
        var u = Unit.new(Team.computer, Globals.EnemyHealth)
        var c = DebugColor.new(Globals.EnemyColor)
        var s = Sprite.new("[games]/jump-core/images/ships/Enemies-6b.png")
        s.layer = 0.9
        s.flags = Render.spriteCenter
        ship.addComponent(t)
        ship.addComponent(b)
        ship.addComponent(e)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(s)
        ship.name = "Enemy"
        ship.tag = (Tag.computer | Tag.unit)
        {
            var thrust = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = AnimatedSprite.new("[games]/jump-core/images/ships/thrusters.png", 4, 2, 15)            
            s.layer = 0.999
            s.flags = Render.spriteCenter | Render.spriteFlipX // |
            s.addAnimation("straight", [1, 3, 5, 7])            
            s.addAnimation("diag", [0, 2, 4, 6])
            s.playAnimation("diag")
            s.playAnimation("straight")
            var r = Relation.new(ship)
            r.offset = Vec2.new(20, 0)
            thrust.addComponent(t)
            thrust.addComponent(s)
            thrust.addComponent(r)
        }
        return ship
    }

    static collide(bullets, units) {
        Render.setColor(1.0, 0, 0)
        for(u in units) {
            for(b in bullets) {            
                var uT = u.getComponent(Transform)
                var bT = b.getComponent(Transform)
                var uB = u.getComponent(Body)
                var bB = b.getComponent(Body)
                var dis = uT.position - bT.position
                dis = dis.magnitude
                if(dis < (uB.size + bB.size)) {
                    var unit = u.getComponent(Unit)
                    System.print("Unit %(unit)")             
                    var bullet = b.getComponent(Bullet)
                    System.print("Bullet %(bullet)")
                    unit.damage(bullet.damage)
                    b.delete()
                    // Render.disk(uT.position.x, uT.position.y, 2, 24)
                }
            }
        }
    }

    static checkCollision(e0, e1) {
        var t0 = e0.getComponent(Transform)
        var t1 = e1.getComponent(Transform)
        var b0 = e0.getComponent(Body)
        var b1 = e1.getComponent(Body)
        var dist = t0.position - t1.position
        dist = dist.magnitude
        var collision = dist < (b0.size + b1.size)
        return collision
    }

    static playerShip { __ship }

    static random { __random }

    static addScore(s) {
        __score = __score + s
    }

    static createBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var v = Vec2.new(speed, 0)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.player, damage)
        var s = AnimatedSprite.new("[games]/jump-core/images/projectiles/spark.png", 5, 1, 30)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("anim", [0,1,2,3,4])
        s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.player | Tag.bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }

    static createBulletDirectedEnemy(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var targetPos = playerShip.getComponent(Transform).position
        var dir = targetPos - owt.position
        var v = dir.normalise * speed
        var bd = Body.new(5, v)
        var bl = Bullet.new(owu.team, damage)
        var s = AnimatedSprite.new("[games]/jump-core/images/projectiles/projectile-02.png", 2, 1, 10)
        s.layer = 0.9
        s.flags = Render.spriteCenter
        s.addAnimation("fly", [0,1])
        s.playAnimation("fly")
        s.idx = 0
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet2"
        bullet.tag = Tag.computer | Tag.bullet
        bullet.addComponent(DebugColor.new(0xEC468BFF))
    }

    static createBulletStraightEnemy(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var v = Vec2.new(-speed, 0)
        var bd = Body.new(5, v)
        var bl = Bullet.new(owu.team, damage)
        var s = AnimatedSprite.new("[games]/jump-core/images/projectiles/projectile-06-02.png", 1, 3, 10)
        s.layer = 0.9
        s.flags = Render.spriteCenter
        s.addAnimation("fly", [0,1,2])
        s.playAnimation("fly")
        s.idx = 0
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet2"
        bullet.tag = Tag.computer | Tag.bullet
        bullet.addComponent(DebugColor.new(0xEC468BFF))
    }

    static createBulletSpreadEnemy(owner, speed, damage) {
        var dir = [ 0.0, 0.2, -0.2 ]
        for(i in 0...3) {
            var owt = owner.getComponent(Transform)
            var owu = owner.getComponent(Unit)
            var bullet = Entity.new()
            var t = Transform.new(owt.position)
            var d = -Vec2.new(dir[i].cos, dir[i].sin)
            var v = d.normalise * speed
            var bd = Body.new(5, v)
            var bl = Bullet.new(owu.team, damage)
            var s = AnimatedSprite.new("[games]/jump-core/images/projectiles/projectile-04.png", 2, 1, 10)
            s.layer = 0.9
            s.flags = Render.spriteCenter
            s.addAnimation("fly", [0,1])
            s.playAnimation("fly")
            s.idx = 0
            bullet.addComponent(t)
            bullet.addComponent(bd)
            bullet.addComponent(bl)
            bullet.addComponent(s)
            bullet.name = "Bullet2"
            bullet.tag = Tag.computer | Tag.bullet // |
            bullet.addComponent(DebugColor.new(0xEC468BFF))
        }
    }

    static createExplosion(owner) {
        var owt = owner.getComponent(Transform)
        var explosion = Entity.new()
        var t = Transform.new(owt.position)
        var b = Body.new(001, Vec2.new(0, 0))
        var e = Explosion.new(1.0)
        var s = AnimatedSprite.new("[games]/jump-core/images/vfx/explosion-a.png", 8, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("explode", [0, 1, 2, 3, 4, 5, 6, 7])
        s.playAnimation("explode")
        s.mode = AnimatedSprite.destroy
        explosion.addComponent(t)
        explosion.addComponent(b)
        explosion.addComponent(e)
        explosion.addComponent(s)
        explosion.addComponent(DebugColor.new(0xFFFFFFFF))
        explosion.name = "Explosion"
    }
}

import "ships" for Orbitor, Shield, EnemyCore, Enemy
import "unit" for Unit
import "player" for Player
import "portal" for Portal
import "ui" for MainMenu, ScoreMenu
