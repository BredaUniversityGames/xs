import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart
import "bullets" for Bullet

class Cannon is BossPart {
    construct new(level) {
        super(level)
        _time = 0        
    }
    
    shoot() {
        var t = (level / 2).floor
        t = 1.5 * Data.getNumber("Cannon Spread") * -t
        var dt = Data.getNumber("Cannon Spread")
        for(i in 0...level) {
            Create.enemyBullet(
                owner,
                -Data.getNumber("Cannon Round Speed"),
                Data.getNumber("Cannon Damage"),
                Vec2.new(t, 0))
            t = t + dt
        }
    }
}

import "create" for Create
