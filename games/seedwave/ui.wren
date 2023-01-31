import "xs" for Data, Render
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Sprite, Relation, GridSprite
import "xs_math" for Vec2, Color

class HealthBar is Component {
    construct new(target) {
        _target = target
    }

    initialize() {        
        _gridSprite = owner.getComponent(GridSprite)
    }

    update(dt) {
        var max = _target.maxHealth
        var cur = _target.health
        var per = cur / max
        var idx = (100 - per * 100).round
        if(idx > 99) {
            idx = 99
        } else if(idx < 0) {
            idx = 0
        }                        
        _gridSprite.idx = idx
    }
}

import "game" for Game
import "unit" for Unit