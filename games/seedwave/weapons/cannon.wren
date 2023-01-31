import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart
import "bullets" for Bullet

class Cannon is BossPart {
    construct new() {
        super()
        _time = 0        
    }

    /*
    update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Cannon Shoot Time")) {
            Bullet.create(owner, -Data.getNumber("Cannon Round Speed"), 0)
            _time = 0
        }

        super.update(dt)
    }
    */
    
    shoot() {
        Bullet.create(owner, -Data.getNumber("Cannon Round Speed"), 0)
    }
}
