import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart

class Vulcan is BossPart {
    construct new(level) {
        super(level)
        _time = 0        
    }
    
    shoot() {        
        var p = Game.player
        var d = p.getComponent(Transform).position - owner.getComponent(Transform).position
        d = d.normal
        var dv = d * Data.getNumber("Vulcan Speed")

        // var t = (level / 2).floor
        var t = 0
        // t = 1.5 * Data.getNumber("Vulcan Spread") * -t
        for(i in 0...level) {
            Create.enemyBullet(
                owner,
                dv,
                Data.getNumber("Cannon Damage"),
                Vec2.new(t, 0))
        }
    }
}

import "create" for Create
import "game" for Game