import "xs" for Configuration, Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "debug" for DebugColor
import "components" for SlowRelation
import "random" for Random

class Bullet is Component {
    construct new(team, damage) {
        super()
        _team = team
        _damage = damage
    }

    update(dt) {
        var t = owner.getComponent(Transform)
        var w = Configuration.width * 0.45
        var h = Configuration.height * 0.45

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


    static create(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = Vec2.new(0, speed)
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
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFFA8D3FF))
    }
}

class Missile is Bullet {
    construct new(team) {
        //var d = Data.getNumber("Missle Damage")
        //super(d)
        super(team, 100)
        // _target = Game.player
    }

    update(dt) {
        var p = Game.player
        var d = p.getComponent(Transform).position - owner.getComponent(Transform).position
        d = d.normalise
        var b = owner.getComponent(Body)
        b.velocity = d * 200.0 
        super.update(dt)
    }


    static create(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = Vec2.new(0, speed)
        var bd = Body.new(5, v)
        var bl = Missile.new(Team.Computer)
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
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFDFFC1FF))
    }

    toString { "[Missile] ->" + super.toString() }
}

import "game" for Game