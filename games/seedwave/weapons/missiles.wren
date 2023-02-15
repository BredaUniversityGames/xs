import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart
import "bullets" for Missile

class Missiles is BossPart {
    construct new(level) {
        super(level)
        _time = 0
    }

    shoot() {
        Create.missile(owner,
            Data.getNumber("Missle Speed"),
            Data.getNumber("Missle Damage"))
    }

    toString { "[Missiles _time:%(_time)] ->" + super.toString }
}

import "create" for Create