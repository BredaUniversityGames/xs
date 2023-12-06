import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

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


class Missile is Bullet {
    construct new(team, target, speed, position) {
        super(team, Data.getNumber("Missle Damage"))
        _speed = Data.getNumber("Missle Speed")
        _time  = 0
        _target = target
        
        _P0 = position
        _P1 = _P0 + Vec2.new(-100, 100)
        var P2 = _target.getComponent(Transform).position

        var dist01 = _P1 - _P0
        var dist12 = P2 - _P1
        var d = dist01.magnitude + dist12.magnitude
        _life = d / _speed
    }

    update(dt) {
        var t = owner.getComponent(Transform)
        _time = _time + dt
        var P2 = _target.getComponent(Transform).position

        var param = _time / _life
        var pos = Math.quadraticBezier(_P0, _P1, P2, param)
        var predPos = Math.quadraticBezier(_P0, _P1, P2, param + 0.1)
        var d = predPos - pos
        d = d.normal
        var alpha = d.atan2 - Math.pi * 0.5
        t.rotation = alpha
        t.position = pos
        
        if(param > 1.2) {
            owner.delete()
        }
    }

    toString { "[Missile speed %(_speed) time%(_time) ] ->" + super.toString }
}
