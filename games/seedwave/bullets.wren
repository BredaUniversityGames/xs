import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

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
}

class Missile is Bullet {
    construct new(team, speed) {
        super(team, Data.getNumber("Missle Damage"))
        _speed = speed
        _targeting = 3.0
        _time  = 0
    }

    update(dt) {        
        if(_targeting > 0) {
            _targeting = _targeting - dt * 2.0
        } else {
            _targeting = 0
        }
        
        //_speed = Math.damp(_speed, Data.getNumber("Missle Max Speed"), 0.5, dt)        

        var p = Game.player
        var b = owner.getComponent(Body)
        var d = p.getComponent(Transform).position - owner.getComponent(Transform).position
        d = d.normal
        var dv = d * Data.getNumber("Missle Max Speed")
        dv = dv - b.velocity
            
        b.velocity = Math.damp(b.velocity, dv, _targeting, dt)                

        if(b.velocity.magnitude < Data.getNumber("Missle Max Speed") * 0.4) {
           b.velocity = b.velocity.normal
           b.velocity = b.velocity * Data.getNumber("Missle Max Speed") * 0.4
        }

        // b.velocity = Math.damp(b.velocity, d, 5, dt) 
        // b.velocity = b.velocity * b.velocity.normal.dot(d)

        var alpha = b.velocity.atan2 + Math.pi * 0.5
        owner.getComponent(Transform).rotation = alpha

        _time = _time + dt
        if(_time > 5.0) {
            owner.delete()
        }
        
        super.update(dt)
    }

    toString { "[Missile] ->" + super.toString }
}

import "game" for Game
import "unit" for Unit
import "tags" for Team, Tag
import "debug" for DebugColor
import "components" for SlowRelation, TurnToVelocity
import "random" for Random