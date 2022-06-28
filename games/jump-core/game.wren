import "xs" for Configuration, Input, Render, Registry, Color, File
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "random" for Random
import "globals" for Globals
import "tags" for Team, Tag
import "debug" for DebugColor
import "ui" for Menu

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

        var w = Configuration.width * 0.5
        if (t.position.x < -w) {
            owner.delete()
        } else if (t.position.x > w) {
            owner.delete()
        }

        var h = Configuration.height * 0.5
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

class Parallax is Component {
    construct new(speed, width, repeat) {
        _speed = speed
        _width = width
        _repeat = repeat
    }

    update(dt) {        
        var t = owner.getComponent(Transform)
        t.position.x = t.position.x - dt * _speed

        if(t.position.x < Configuration.width * -0.5 -_width) {
            t.position.x = t.position.x + _width * _repeat
        }
    }

}

class GameState {
    static Title    { 0 }
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {
    static config() {        
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 2
        Configuration.title = "SubOptimal"
    }

    static init() {        
        Entity.init()
        createBackground()
        createTitle()

        __frame = 0
        __random = Random.new()
        __state = GameState.Title        
        __score = 0
        __waveTimer = 0
        __wave = 0
        __shakeOffset = Vec2.new(0, 0)
        __shakeIntesity = 0

        __font = Render.loadFont("[games]/jump-core/fonts/FutilePro.ttf", 18)

        // startPlay()
    }        
    
    static update(dt) {        
        Entity.update(dt)
        if(__state == GameState.Title) {
            updateTitle(dt)
        } else  if(__state == GameState.Menu) {
            __menu.update(dt)
        } else if(__state == GameState.Play) {
            updatePlay(dt)
        } else if(__state == GameState.Score) {
            updateScore(dt)
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
            var text = "SCORE %(__score)   |  WAVE %(__wave)  |  HEALTH %(pu.health)"
            Render.renderText(__font, text, 0, 150, 0xFFFFFFFF, 0xFFFFFFFF, Render.spriteCenter)
        } else if(__state == GameState.Menu) {
            __menu.render(100.0, 0.0)
        }
    }

    static startPlay() {
        __score = 0
        __waveTimer = 0
        __wave = 0
        __state = GameState.Play
        createPlayerShip()        
    }

    static updateTitle(dt) {
        if (Input.getButtonOnce(0) == true || Input.getKeyOnce(Input.keySpace)) {
            startMenu()
        }
    }

    static updatePlay(dt) {
        var pu = playerShip.getComponent(Unit)
        //Render.setColor(0xFFFFFFFF)
        //Render.text("SCORE %(__score)   |  WAVE %(__wave)  |  HEALTH %(pu.health)", -296, 170, 1)

        /*
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
        */
 
        var playerUnits = Entity.entitiesWithTag(Tag.player | Tag.unit)
        var playerBullets = Entity.entitiesWithTag(Tag.player | Tag.bullet)
        var computerUnits = Entity.entitiesWithTag(Tag.computer | Tag.unit)
        var computerBullets = Entity.entitiesWithTag(Tag.computer | Tag.bullet)
        // var computerBullets = Entity.entitiesWithTag(Tag.computer | Tag.bullet)

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
            __state = GameState.Score
        }

        /*
        __frame = __frame + 1
        if(__frame == 100) {
            Entity.print()
            __frame = 0
        }
        */
    }

    static startMenu() {
        __title.delete()
        __title = null
        __state = GameState.Menu
        __menu = Menu.new(["play", "options", "credits", "exit"])
        __menu.addAction("play", Fn.new { Game.startPlay() } )

    }

    static updateScore(dt) {
        Render.setColor(0x8BEC46FF)
        Render.text("SCORE %(__score)", -100, 50, 4)
        Render.text(">menu<", -50, 10, 2)

        if (Input.getButtonOnce(0) || Input.getKeyOnce(Input.keySpace)) {
            __state = GameState.Menu            
        } 
    }

    static addShake() {
        __shakeIntesity = 4.0
    }

    static createBackground() {
        var layers = [
            "[games]/jump-core/images/backgrounds/abandoned/daytime_background.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud01.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud02.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud03.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud04.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud05.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud06.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud07.png",
            "[games]/jump-core/images/backgrounds/abandoned/daytime_cloud08.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_buildingsback.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_buildingsfront.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building01.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building02.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building03.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building04.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building05.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building06.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building07.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_building08.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_train.png",
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_traintracks.png",            
            "[games]/jump-core/images/backgrounds/abandoned/abandoned_trees.png",
        ]

        var widths = [
            320,    // bg
            520,    // cl 1
            520,    // cl 2
            520,    // cl 3
            520,    // cl 4
            520,    // cl 5
            520,    // cl 6
            520,    // cl 7
            520,    // cl 8
            320,    // bl bk
            320,    // bl f
            520,    // b1
            520,    // b2
            520,    // b3
            520,    // b4
            520,    // b5
            520,    // b6
            520,    // b7
            520,    // b8
            720,    // train
            320,    // train tr
            320,    // trees
        ]

        var speeds = [
            0,    // bg
            25.0,   // cl 1
            25.0,   // cl 2
            22.0,   // cl 3
            16.0,   // cl 4
            12.0,   // cl 5
            13.0,   // cl 6
            15.0,   // cl 7
            15.0,   // cl 8
            40,   // bl bk
            80,   // bl f
            100,    // b1
            100,    // b2
            100,    // b3
            100,    // b4
            100,    // b5
            100,    // b6
            100,    // b7
            100,    // b8
            120,  // train t
            120,  // train
            140   // trees
        ]

        var heights = [
            0,    // bg
            220,  // cl 1
            130,  // cl 2
            230,  // cl 3
            170,  // cl 4
            120,  // cl 5
            120,  // cl 6
            170,  // cl 7
            170,  // cl 8
            0,    // bl bg    
            0,    // bl f
            0,      // b1
            0,      // b2
            0,      // b3
            0,      // b4
            0,      // b5
            0,      // b6
            0,      // b7
            0,      // b8
            26.0,   // train 
            0,    // train tr
            0     // trees
        ]

        var starts = [
            0,    // bg
            30,    // cl 1
            -10,    // cl 2
            280,    // cl 3
            130,    // cl 4
            170,    // cl 5
            220,    // cl 6
            90,    // cl 7
            290,    // cl 8
            0,    // bl bg
            0,    // bl front
            100,      // b1
            150,      // b2
            200,      // b3
            250,      // b4
            300,      // b5
            350,      // b6
            400,      // b7
            500,      // b8
            0,    // train 
            0,    // train tr
            0,    // trees
        ]

        var w = Configuration.width * 0.5
        var h = Configuration.height * 0.5

        for(i in 0...22) {
            for(j in 0..3) {
                var parallax = Entity.new()
                var t = Transform.new(
                    Vec2.new(-w + widths[i] * j + starts[i],
                    -h + heights[i]))
                var s = Sprite.new(layers[i], 0, 0, 1.0, 1.0)
                var p = Parallax.new(speeds[i], widths[i], 3)
                parallax.addComponent(t)
                parallax.addComponent(s)
                parallax.addComponent(p)
            }
        }
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
            var r  = Relation.new(__title)
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
            var t = Transform.new(Vec2.new(75,20))
            var r = Relation.new(__title)
            r.offset = Vec2.new(75,20)
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
    }

    static createPlayerShip() {
        var ship = Entity.new()            
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()
        var v = Vec2.new(0, 0)
        var b = Body.new( Globals.PlayerSize , v)
        var u = Unit.new(Team.player, Globals.PlayerHealth)
        var c = DebugColor.new(0x8BEC46FF)
        var o = Orbitor.new(ship)
        var s = GridSprite.new("[games]/jump-core/images/ships/blue-ship-spritesheet.png", 5, 1)
        s.layer = 1.0
        s.flags = Render.spriteCenter
        // s.addAnimation("fly", [0,1,2])
        // s.playAnimation("fly")
        ship.addComponent(t)
        ship.addComponent(sc)            
        ship.addComponent(b)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(o)
        ship.addComponent(s)
        ship.name = "Player"
        ship.tag = (Tag.player | Tag.unit)
        __ship = ship
        {
            var thrust = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = AnimatedSprite.new("[games]/jump-core/images/ships/thrusters.png", 4, 2, 15)            
            s.layer = 0.999
            s.flags = Render.spriteCenter
            s.addAnimation("straight", [1, 3, 5, 7])            
            //s.addAnimation("diag", [0, 2, 4, 6])
            //s.playAnimation("diag")            
            s.playAnimation("straight")
            var r = Relation.new(ship)
            r.offset = Vec2.new(-20, 0)
            thrust.addComponent(t)
            thrust.addComponent(s)
            thrust.addComponent(r)
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
        var s = Sprite.new("[games]/jump-core/images/ships/Purple-4.png")
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
                    u.getComponent(Unit).damage(b.getComponent(Bullet).damage)
                    b.delete()
                    // Render.disk(uT.position.x, uT.position.y, 2, 24)
                }
            }
        }
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
        var s = AnimatedSprite.new("[games]/jump-core/images/vfx/Explosion.png", 8, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("explode", [0,1,2, 3, 4, 5, 6, 7])
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
