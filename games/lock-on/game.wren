import "xs" for Input, Render, Data, File, Audio
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation, Label
import "random" for Random
import "globals" for Globals
import "tags" for Team, Tag
import "debug" for DebugColor
import "background" for Background
import "projectiles" for Bullet, Missile, BulletType

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
        //Data.setNumber("Multiplier", 1, Data.system)
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
        __font = Render.loadFont("[game]/fonts/FutilePro.ttf", 18)
        __levels = ["daytime", "night", "abandoned", "snow-rain", "sunset"]

        // var song = Audio.load("[game]/Blast_2019.flac", Audio.groupMusic)
        // var sound = Audio.play(song)
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

            var text = "SCORE %(__score)   :  WAVE %(__wave)  :  HEALTH %(pu.health)"
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
        //__menu = null
    }

    static updateTitle(dt) {
        if (Input.getButtonOnce(0) == true || Input.getKeyOnce(Input.keySpace)) {
            startMenu()
        }
    }

    static updatePlay(dt) {
        var pu = playerShip.getComponent(Unit)
        __time = __time + dt

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

        if(Data.getBool("Print Entities", Data.debug)) {
            Entity.print()
            Data.setBool("Print Entities", false, Data.debug)            
        }
 
        var playerUnits = Entity.withTag(Tag.player | Tag.unit)
        var playerBullets = Entity.withTag(Tag.player | Tag.bullet)
        var computerUnits = Entity.withTag(Tag.computer | Tag.unit)
        var computerBullets = Entity.withTag(Tag.computer | Tag.bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)

        // Lock on to enemies
        var reticle = Entity.withTag(Tag.reticle)
        if(reticle.count > 0) {
            reticle = reticle[0]
            var rt = reticle.getComponent(Transform)            
            var closest = null
            var closestDis = 999999999.9            
            var shipTransform = __ship.getComponent(Transform)
            var reticlePosition = shipTransform.position + Vec2.new(200, 0)

            for (cu in computerUnits) {
                var ct = cu.getComponent(Transform)
                var cb = cu.getComponent(Body)
                var dis = reticlePosition - ct.position
                dis = dis.magnitude
                if(dis < closestDis) {
                    closest = cu
                    closestDis = dis
                }
            }

            var reticleSnapDist = Data.getNumber("Reticle Snap Distance", Data.game)
            var rtPosition = null
            if(closest != null) {
                if(closestDis < reticleSnapDist) {
                    var target = closest.getComponent(Target)
                    target.lock(dt)
                    var ct = closest.getComponent(Transform)
                    Render.arc(ct.position.x, ct.position.y, 20, target.lock * Math.pi, 24)
                    rtPosition = ct.position
                } else {
                    rtPosition = reticlePosition
                }
            } else {
                rtPosition = reticlePosition
            }

            var reticleDampLambda = Data.getNumber("Reticle Damp Lambda", Data.game)
            rt.position = Math.damp(rt.position, rtPosition, reticleDampLambda, dt)
        }                

        if(computerUnits.count == 0) {
            __waveTimer = __waveTimer + dt            
        }

        if(__waveTimer >= 1.5) {
            //for(w in 0..__wave) {
                createEnemyShips()
            //}
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
            var s = Sprite.new("[game]/images/backgrounds/title.png", 0, 0, 1, 1)
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
            var s = AnimatedSprite.new("[game]/images/vfx/Electric_Effect_05.png", 4, 4, 15)
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
            var l = Label.new("[game]/fonts/FutilePro.ttf", "Press any button to start", 18)
            l.layer = 11.0
            l.flags = Render.spriteCenter                                    
            e.name = "Any Button"
            e.addComponent(t)
            e.addComponent(l)
            e.addComponent(r)
        }
    }

    static createEnemyShips() {
        for(i in 0...3) {
            var x = Game.random.float(0, 200)
            var y = Game.random.float(-100, 100)
            var bulletType = Game.random.int(1, 4) 
            var ship = createEnemyShip(x, y, bulletType)
        }
    }


    static createEnemyShip(x, y, bulletType) {
        var ship = Entity.new()

        { // Ship
            var p = Vec2.new(x, y)
            var t = Transform.new(p)    
            var v = Vec2.new(0, 0)
            var b = Body.new(Globals.EnemySize, v)
            var e = Enemy.new(bulletType)
            var u = Unit.new(Team.computer, Globals.EnemyHealth)
            var c = DebugColor.new(Globals.EnemyColor)
            var s = null
            if(bulletType == BulletType.directed) {
                s = Sprite.new("[game]/images/ships/Enemies-5b.png")
            } else if(bulletType == BulletType.spread) {
                s = Sprite.new("[game]/images/ships/Enemies-6b.png")
            } else if(bulletType == BulletType.straight) {
                s = Sprite.new("[game]/images/ships/Enemies-1b.png")
            } else if(bulletType == BulletType.missile) {
            } else {
            }
            
            s.layer = 0.9
            s.flags = Render.spriteCenter
            ship.addComponent(t)
            ship.addComponent(b)
            ship.addComponent(e)
            ship.addComponent(u)
            ship.addComponent(c)
            ship.addComponent(s)
            ship.name = "Enemy"
            ship.tag = (Tag.computer | Tag.unit) // |
        }

        { // Thrust
            var thrust = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = AnimatedSprite.new("[game]/images/ships/thrusters.png", 4, 2, 15)            
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

        var lockSprite = null 
        { // Target animation
            var target = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            lockSprite = GridSprite.new("[game]/images/ui/lock.png", 33, 1)
            lockSprite.layer = 10.0
            lockSprite.flags = Render.spriteCenter
            var r = Relation.new(ship)
            r.offset = Vec2.new(0, 0)
            target.addComponent(t)
            target.addComponent(lockSprite)
            target.addComponent(r)
        }

        var target = Target.new(lockSprite)
        ship.addComponent(target)

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
                    var bullet = b.getComponentSuper(Bullet)
                    System.print("Bullet %(bullet)")
                    unit.damage(bullet.damage)
                    b.delete()
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
        var s = AnimatedSprite.new("[game]/images/projectiles/spark.png", 5, 1, 30)
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

    static createMissile(target) {
        var owner = __ship
        var owt = owner.getComponent(Transform)

        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var targetPos = target.getComponent(Transform).position
        var dir = targetPos - owt.position
        var spd = Data.getNumber("Missile Launch Speed", Data.game)
        var v = Vec2.new(-1.0, Game.random.float(-1.0, 1.0))
        v = v.normal
        v = v * spd
        var bd = Body.new(5, v)
        var bl = Missile.new(Team.player, target, 10.0)
        var s = AnimatedSprite.new("[game]/images/projectiles/missile-02.png", 3, 1, 20)
        s.layer = 0.9
        s.flags = Render.spriteCenter
        s.addAnimation("fly", [0, 1, 2])
        s.playAnimation("fly")
        s.idx = Game.random.int(0, 2)
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Missile"
        bullet.tag = Tag.player | Tag.bullet | Tag.missile // |
        bullet.addComponent(DebugColor.new(0xEC468BFF))

    }

    static createMissilesForAllTargets() {
        var missiles = Entity.withTag(Tag.player | Tag.missile)
        if(missiles.count > 0) {
            return
        }

        var targets = Entity.withTag(Tag.computer | Tag.unit)
        for(t in targets) {
            var target = t.getComponent(Target)
            if(target.locked) {
                Game.createMissile(t)
            }
        }
    }

    static createBulletDirectedEnemy(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var targetPos = playerShip.getComponent(Transform).position
        var dir = targetPos - owt.position
        var v = dir.normal * speed
        var bd = Body.new(5, v)
        var bl = Bullet.new(owu.team, damage)
        var s = AnimatedSprite.new("[game]/images/vfx/blue_effects.png", 20, 16, 20)
        s.layer = 0.9
        s.flags = Render.spriteCenter
        s.addAnimation("fly", [71, 72, 73, 74])
        s.playAnimation("fly")
        s.idx = Game.random.int(71, 74)
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
        var s = AnimatedSprite.new("[game]/images/vfx/blue_effects.png", 20, 16, 20)
        s.layer = 0.9
        s.flags = Render.spriteCenter
        s.addAnimation("fly", [71 + 40, 72 + 40, 73 + 40, 74 + 40])
        s.playAnimation("fly")
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
            var v = d.normal * speed
            var bd = Body.new(5, v)
            var bl = Bullet.new(owu.team, damage)
            var s = AnimatedSprite.new("[game]/images/projectiles/projectile-04.png", 2, 1, 10)
            s.layer = 0.9
            s.flags = Render.spriteCenter
            s.addAnimation("fly", [0,1])
            s.playAnimation("fly")
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
        var s = AnimatedSprite.new("[game]/images/vfx/explosion-a.png", 8, 1, 15)
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

import "ships" for Enemy, Target
import "unit" for Unit
import "player" for Player
import "ui" for MainMenu, ScoreMenu
