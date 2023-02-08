import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Create {

    static init() {
        __random = Random.new()
    }

    static drone(offset, parent) {        
        var e = Entity.new()
        var t = Transform.new(Vec2.new())
        // var v = Vec2.new(0, 0)
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

    static Turret(parent, position, size, type, power) {
        var e = Entity.new()
        var t = Transform.new(Vec2.new())
        var r = Relation.new(parent)
        var tr = null
        var s = null
        if (size == Size.S) {
            s = GridSprite.new("[game]/assets/images/Weapons/Small/turret_small_32x32.png", 3, 4)
        } else if(size == Size.M) {
            s = GridSprite.new("[game]/assets/images/Weapons/Medium/turret_medium_64x64.png", 3, 4)
        }

        if(type == Turret.Cannon) {
            s.idx = 3 * 3 + power
            tr = Cannon.new()
        } else if(type == Turret.Launcher) {
            s.idx = 3 + power
            tr = Launcher.new()
        } else if(type == Turret.Laser) {
            s.idx = power
            tr = Laser.new()
        } else if(type == Turret.Plasma) {
            s.idx = 2 * 3 + power
            tr = Plasma.new()
        }

        s.flags = Render.spriteCenter
        r.offset = position

        e.addComponent(t)
        e.addComponent(r)
        e.addComponent(s)
        //e.addComponent(tr)
    }

    static Turret(parent, position, size) {
        var type = __random.int(0, 5)
        var power = __random.int(0, 3)
        Create.Turret(parent, position, size, type, power)
    }



    static EnemySmall(entity, variation) {
        var s = GridSprite.new("[game]/assets/images/Ships/Small/ship_small_32x64.png", 3, 1)
        s.idx = variation
        var c = Body.new(4, Vec2.new())
        s.flags = Render.spriteCenter
        entity.addComponent(s)
        entity.addComponent(c)
        Create.Turret(entity, Vec2.new(), Size.S)

        Create.Exhaust(entity, Vec2.new(0, 33), 0.0, 1)
    }

    static EnemyMedium(entity, variation) {
        var s = GridSprite.new("[game]/assets/images/Ships/Medium/ship_medium_64x128.png", 5, 1)
        s.idx = variation
        var c = Body.new(4, Vec2.new())
        s.flags = Render.spriteCenter
        entity.addComponent(s)
        entity.addComponent(c)
        Create.Turret(entity, Vec2.new(), Size.M)

        if(variation != 3) {
            Create.Exhaust(entity, Vec2.new(20, 63), Math.radians(-45.0), 1.2)
            Create.Exhaust(entity, Vec2.new(-20, 63), Math.radians(45.0), 1.2)
        } else {
            Create.Exhaust(entity, Vec2.new(16, 68), 0.0, 1)
            Create.Exhaust(entity, Vec2.new(0, 68), 0.0, 1)
            Create.Exhaust(entity, Vec2.new(-16, 68), 0.0, 1)
        }
    }

    static EnemyLarge(entity, variation) {
        var s = GridSprite.new("[game]/assets/images/Ships/Big/ship_big_128x256.png", 4, 1)
        var c = Body.new(4, Vec2.new())
        s.flags = Render.spriteCenter
        s.idx = variation
        entity.addComponent(s)
        entity.addComponent(c)

        Create.Exhaust(entity, Vec2.new(36, 126), Math.radians(-45.0), 2)
        Create.Exhaust(entity, Vec2.new(-36, 126), Math.radians(45.0), 2)

        if(variation == 0) {
            Create.Turret(entity, Vec2.new(16.0, 16.0), Size.S)
            Create.Turret(entity, Vec2.new(-16.0, 16.0), Size.S)
            Create.Turret(entity, Vec2.new(16.0, -16.0), Size.S)
            Create.Turret(entity, Vec2.new(-16.0, -16.0), Size.S)
            Create.Turret(entity, Vec2.new(16.0, -48.0), Size.S)
            Create.Turret(entity, Vec2.new(-16.0, -48.0), Size.S)
        } else if(variation == 1) {
            Create.Turret(entity, Vec2.new(), Size.M)
            Create.Turret(entity, Vec2.new(0, -70), Size.M)
        } else if(variation == 2) {
            Create.Turret(entity, Vec2.new(0.0, 16.0), Size.M)
            Create.Turret(entity, Vec2.new(0, -36), Size.S)
            Create.Turret(entity, Vec2.new(0, -70), Size.S)
        } 
    }

    static Enemy(position, size, variation) {
        var e = Entity.new()
        var t = Transform.new(position)
        var s = Ship.new()
        e.addComponent(t)
        e.addComponent(s)
        if(size == Size.S) {
            Create.EnemySmall(e, variation)
        } else if (size == Size.M) {
            Create.EnemyMedium(e, variation)
        } else if(size == Size.L) {
            Create.EnemyLarge(e, variation)
        }
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

    static PlayerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0,10))
        var dir =  Vec2.new(__random.float(-0.2, 0.2), 1.0)
        dir = dir.normal
        dir = dir * speed
        var v = dir
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var s = AnimatedSprite.new("[games]/seedwave/assets/images/projectiles/projectile-06-02.png", 3, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
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
        var v = direction * 2
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Computer, damage)
        var s = AnimatedSprite.new("[game]/assets/images/Projectiles_2/projectile-04.png", 2, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
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
}

import "unit" for Unit
import "components" for SlowRelation
import "tags" for Team, Tag, Size
import "bullets" for Bullet
import "debug" for DebugColor
import "random" for Random
import "enemy" for Turret, Ship, Cannon, Laser, Launcher, Plasma
import "player" for Player
import "game" for Game
