import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "debug" for DebugColor
import "components" for SlowRelation, TurnToVelocity
import "random" for Random

class Bullet is Component {
    construct new(team, damage) {
        super()
        _team = team
        _damage = damage
    }

    update(dt) {
        var t = owner.getComponent(Transform)
        var w = Data.getNumber("Width", Data.system) * 0.51
        var h = Data.getNumber("Height", Data.system) * 0.51

        if (t.position.x < -w) {
            owner.delete()
        } else if (t.position.x > w) {
            owner.delete()
        }
        
        if (t.position.y < -h) {
            owner.delete()
        } else if (t.position.y > h) {
            owner.delete()
        }
    }

    damage { _damage }
    team { _team }
    toString { "[Bullet team:%(_team) damage:%(_damage)]" }

    static createPlayerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0,10))
        var dir =  Vec2.new(Game.random.float(-0.2, 0.2), 1.0)
        dir = dir.normalise
        dir = dir * speed
        var v = dir
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var tr = TurnToVelocity.new()
        var s = Sprite.new("[game]/assets/images/projectiles/pl_cannon.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        //s.addAnimation("anim", [0,1,2])
        //s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.addComponent(tr)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }

    static create(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = Vec2.new(0, speed)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var s = Sprite.new("[game]/assets/images/projectiles/cannon.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        //s.addAnimation("anim", [0,1,2])
        //s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFFA8D3FF))
    }
}

class Missile is Bullet {
    construct new(team, speed) {
        //var d = Data.getNumber("Missle Damage")
        //super(d)
        super(team, 100)
        _speed = speed
        _targeting = 1.5
        _time  = 0
        // _target = Game.player
    }

    update(dt) {        
        if(_targeting > 0) {
            _targeting = _targeting - dt * 0.4
        } else {
            _targeting = 0
        }
        
        //_speed = Math.damp(_speed, Data.getNumber("Missle Max Speed"), 0.5, dt)        

        var p = Game.player
        var b = owner.getComponent(Body)
        var d = p.getComponent(Transform).position - owner.getComponent(Transform).position
        d = d.normalise
        var dv = d * Data.getNumber("Missle Max Speed")
        dv = dv - b.velocity

        b.velocity = Math.damp(b.velocity, dv, _targeting, dt)                

        // b.velocity = Math.damp(b.velocity, d, 5, dt) 
        // b.velocity = b.velocity * b.velocity.normalise.dot(d)

        var alpha = b.velocity.atan2 + Math.pi * 0.5
        owner.getComponent(Transform).rotation = alpha

        _time = _time + dt
        if(_time > 5.0) {
            owner.delete()
        }
        
        super.update(dt)
    }

    static create(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = Vec2.randomDirection()        
        v = v.normalise * speed   
        if(v.y < 0) {
            v.y = -v.y
        }

        var bd = Body.new(5, v)
        var bl = Missile.new(Team.Computer, speed)
        var u = Unit.new(Team.Computer, 1.0, true)
        var s = Sprite.new("[game]/assets/images/projectiles/missile.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        // s.addAnimation("anim", [0,1,2])
        // s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.addComponent(u)
        bullet.name = "Bullet"
        bullet.tag = Tag.Computer | Tag.Bullet | Tag.Unit
        bullet.addComponent(DebugColor.new(0xFDFFC1FF))
    }

    toString { "[Missile] ->" + super.toString }
}

import "game" for Game