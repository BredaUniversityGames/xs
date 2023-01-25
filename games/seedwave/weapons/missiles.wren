import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart
import "bullets" for Missile

class Missiles is BossPart {
    construct new() {
        super()
        _time = 0
    }

     update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Missle Shoot Time")) {            
            Missile.create(owner,
                Data.getNumber("Missle Speed"),
                Data.getNumber("Missle Damage"))
            _time = 0
        }

        super.update(dt)
    }

    shoot() {
    }

    toString { "[Missiles _time:%(_time)] ->" + super.toString }
}