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
        Create.enemyBullet(
            owner,
            -Data.getNumber("Cannon Round Speed"),
            Data.getNumber("Cannon Damage"))
    }
}

import "create" for Create
