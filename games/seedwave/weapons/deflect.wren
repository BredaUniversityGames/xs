import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart

class Deflect is BossPart {
    construct new() {
        super()
        _time = 0
    }

    /*

    update(dt) {
        super.update(dt)
    }

    shoot() {
    }
    */

    toString { "[Deflect] ->" + super.toString }
}