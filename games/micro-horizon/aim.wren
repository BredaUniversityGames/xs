import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

// The arrow aiming cone
class Aim is Component {
    construct new() {
        super()
    }

    initialize() {
        _relation = owner.getComponent(Relation)
        _transform = owner.getComponent(Transform)
        _sprite = owner.getComponent(GridSprite)
    }

    update(dt) {
        var t = _relation.parent.getComponent(Transform)            
        _transform.rotation = t.rotation

        var b = Entity.withTag(Tag.Bow)
        if(b.count > 0) {
            b = b[0]
            var a = b.getComponent(Arrow)
            _sprite.idx = (a.pull * 8).floor
        }
    }
}

import "game" for Game
import "unit" for Unit
import "tags" for Team, Tag
import "arrow" for Arrow
import "debug" for DebugColor