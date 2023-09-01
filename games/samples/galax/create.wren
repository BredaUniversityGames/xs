import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Create {

    static makeList(size) {
        var list = []
        for(i in 0...size) {
            list.add(i)
        }
        return list
    }

    static init() {
        __random = Random.new()
    }

    static drone(offset, parent) {        
        var e = Entity.new()
        var t = Transform.new(Vec2.new())
        var c = DebugColor.new(0x8BEC46FF)
        var s = AnimatedSprite.new("[game]/assets/images/Support Units/support_units.png", 4, 3, 15)        
        var r = SlowRelation.new(parent, 4.2)
        r.offset = offset
        s.layer = 1.0
        s.flags = Render.spriteCenter
        s.addAnimation("anim", [0, 1])
        s.playAnimation("anim")
        e.addComponent(t)
        e.addComponent(c)
        e.addComponent(s)
        e.addComponent(r)
        e.name = "Drone"
        //ship.tag = (Tag.Player | Tag.Unit)
        return e
    }

    static pair(a, b) { a * 65536 + b }

    static enemy(position, type, difficulty, formation, t) {
        if(__images == null) {
            __images = {}
            __images[pair(Ship.plasma, 1)] = "[game]/assets/images/02 Heavy/heavy_3.png"
            __images[pair(Ship.plasma, 2)] = "[game]/assets/images/02 Heavy/heavy_1.png"
            __images[pair(Ship.plasma, 3)] = "[game]/assets/images/02 Heavy/heavy_4.png"
            __images[pair(Ship.plasma, 4)] = "[game]/assets/images/02 Heavy/heavy_2.png"

            __images[pair(Ship.fire, 1)] = "[game]/assets/images/03 Danger/danger_3.png"
            __images[pair(Ship.fire, 2)] = "[game]/assets/images/03 Danger/danger_1.png"
            __images[pair(Ship.fire, 3)] = "[game]/assets/images/03 Danger/danger_5.png"
            __images[pair(Ship.fire, 4)] = "[game]/assets/images/03 Danger/danger_6.png"

            __images[pair(Ship.cannon, 1)] = "[game]/assets/images/04 Cannon/cannon_3.png"
            __images[pair(Ship.cannon, 2)] = "[game]/assets/images/04 Cannon/cannon_4.png"
            __images[pair(Ship.cannon, 3)] = "[game]/assets/images/04 Cannon/cannon_5.png"
            __images[pair(Ship.cannon, 4)] = "[game]/assets/images/04 Cannon/cannon_6.png"            
        }
        

        var spf = __images[pair(type, difficulty)]
        var sp = Sprite.new(spf)        
        var e = Entity.new()        
        var tr = Transform.new(position)
        sp.flags = Render.spriteCenter
        var c = Body.new(14, Vec2.new())
        var sh = Ship.new(formation, t)
        var u = Unit.new(Team.Computer, 1)
        e.addComponent(tr)
        e.addComponent(sh)
        e.addComponent(sp)
        e.addComponent(c)
        e.addComponent(u)
        e.tag =  Tag.Computer | Tag.Unit

        return e
    }



    static Exhaust(parent, position, rotation, scale) {
        var e = Entity.new()
        var t = Transform.new(Vec2.new())
        var r = Relation.new(parent)
        var s = AnimatedSprite.new("[game]/assets/images/Ships/Small/Exhaust/exhaust.png", 2, 1, 10)
        s.addAnimation("play", [0, 1])
        s.playAnimation("play")
        s.flags = Render.spriteCenter
        t.rotation = rotation
        s.scale = scale
        r.offset = position
        e.addComponent(t)
        e.addComponent(r)
        e.addComponent(s)
    }

    static playerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0,10))
        var dir =  Vec2.new(__random.float(-0.2, 0.2), 1.0)
        dir = dir.normal
        dir = dir * speed
        var v = dir
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var s = AnimatedSprite.new("[game]/assets/images/projectiles/projectile-06-02.png", 3, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.scale = 1.5
        s.addAnimation("anim", [0,1,2])
        s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }

    static enemyProjectile(owner, direction, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        t.rotation = direction.atan2 - Math.pi * 0.5
        var v = direction * 7.0
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Computer, damage)
        var s = AnimatedSprite.new("[game]/assets/images/Projectiles_2/projectile-04.png", 2, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.scale = 1.5
        s.addAnimation("anim", [0,1])
        s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFFA8D3FF))
    }

    static enemyPlasma(owner, direction, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = direction * 7.0
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Computer, damage)
        var s = AnimatedSprite.new("[game]/assets/images/vfx/explosion-f.png", 8, 1, 20)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.scale = 1.5
        s.addAnimation("anim", makeList(7))
        s.playAnimation("anim") 
        s.randomizeFrame(__random)
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Plasma"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFFA8D3FF))
    }

    static explosion(owner) {
        var owt = owner.getComponent(Transform)
        var explosion = Entity.new()
        var t = Transform.new(owt.position)
        var b = Body.new(001, Vec2.new(0, 0))
        var s = AnimatedSprite.new("[game]/assets/images/Explosion/explosion.png", 4, 3, 30)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("explode", [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11])
        s.playAnimation("explode")
        s.mode = AnimatedSprite.destroy
        explosion.addComponent(t)
        explosion.addComponent(b)
        explosion.addComponent(s)
        explosion.addComponent(DebugColor.new(0xFFFFFFFF))
        explosion.name = "Explosion"
    }

    static createFormation(position) {
        var e = Entity.new()
        var t  = Transform.new(position)
        var f = Formation.new(3, 2, 200, 150)
        e.addComponent(t)
        e.addComponent(f)
    }
}

import "unit" for Unit
import "components" for SlowRelation
import "tags" for Team, Tag, Size
import "debug" for DebugColor
import "random" for Random
import "enemy" for Ship, Formation
import "player" for Player
import "game" for Game
import "bullets" for Bullet
