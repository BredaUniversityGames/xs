import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "weapons/bosspart" for BossPart

class Vulcan is BossPart {
    construct new(level) {
        super(level)
        _time = 0
        _shootsLeft = 0
    }

    update(dt) {
        super.update(dt)
        _time = _time + dt
        if(_shootsLeft > 0) {
            if(_time > Data.getNumber("Vulcan Shoot Interval")) {
                _time = 0
                _shootsLeft = _shootsLeft - 1
                fire()
            }
        }
    }
    
    shoot() {        
        _shootsLeft = level
    }

    fire() {
        var p = Game.player
        var d = p.getComponent(Transform).position - owner.getComponent(Transform).position
        d = d.normal
        var dv = d * Data.getNumber("Vulcan Speed")
        Create.enemyVulcan(
            owner,
            dv,
            Data.getNumber("Vulcan Damage"),
            Vec2.new(0, 0))
    }
        
}

import "create" for Create
import "game" for Game