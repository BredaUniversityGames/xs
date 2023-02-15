
import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class BossPart is Component {
    construct new(level) {
        super()
        _level = level
    }

    initialize() {
        _unit = owner.getComponent(Unit)        
        _sprite = owner.getComponentSuper(Sprite)
    }

    update(dt) {
        if(!destroyed && _unit.health <= 0) {
            owner.tag = 0
            owner.deleteComponent(Body)
            owner.deleteComponent(DebugColor)
            owner.deleteComponent(Unit)
            _sprite.add = 0x00000000
            _sprite.mul = 0x4A4A4AFF
            Create.explosion(owner)
        }
    }

    shoot() {}
    ready { true }
    destroyed { owner.getComponent(Unit) == null }
    wait { 0.25 }
    level { _level }
}

import "unit" for Unit
import "debug" for DebugColor
import "create" for Create
