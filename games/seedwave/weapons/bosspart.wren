
import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class BossPart is Component {
    construct new() {
        super()
        _active = true
    }

    initialize() {
        _unit = owner.getComponent(Unit)        
        _sprite = owner.getComponentSuper(Sprite)
    }

    update(dt) {
        if(_active && _unit.health <= 0) {
            System.print("Deed")
            owner.tag = 0
            owner.deleteComponent(Body)
            owner.deleteComponent(DebugColor)
            owner.deleteComponent(Unit)
            //owner.deleteComponentSuper(BossPart)
            _sprite.add = 0x00000000
            _sprite.mul = 0x4A4A4AFF
            _active = false
        }
    }

    shoot() {}

    active { _active }

    wait { 0.15 }
}

import "unit" for Unit
import "debug" for DebugColor
