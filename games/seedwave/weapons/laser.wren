import "xs" for Data, Render
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Sprite, Relation
import "xs_math" for Vec2, Color
import "weapons/bosspart" for BossPart

class LaserState {
    static idle     { 0 }
    static prepare  { 1 }
    static shoot    { 2 }
    static cooldown { 3 }
}

class Laser is BossPart {
    construct new(level) {
        super(level)
        _time = 0     
        _beam = null           
        _state = LaserState.idle
        _damage = Data.getNumber("Laser Damage/Second " + level.toString)
        _width = Data.getNumber("Laser Width " + level.toString)
        _duration = Data.getNumber("Laser Duration ")
    }

    initialize() {
        super.initialize()
        _transform = owner.getComponent(Transform)
    }

    update(dt) {        
        _time = _time + dt

        if(_state == LaserState.prepare) {
            if(_time > 1.0) {
                _beam.delete()
                makeBeam(true)                
                _state = LaserState.shoot
                _time = 0.0
            }
        } else if(_state == LaserState.shoot) {
            var player = Game.player
            var pt = player.getComponent(Transform)
            var dx = (pt.position.x - _transform.position.x).abs
            if(dx < _width) {
                var pu = player.getComponent(Unit)
                pu.damage(_damage * dt)
            }

            // Do some damage here
            if(_time > _duration) {
                _beam.delete()
                _state = LaserState.cooldown
                _time = 0.0
            }
        } else if(_state == LaserState.cooldown) {
            if(_time > 1.0) {
                _state = LaserState.idle
                _time = 0.0
            }
        }

        super.update(dt)
    }
    
    shoot() {  
        makeBeam(false)
        _state = LaserState.prepare
        _time = 0.0
    }

    makeBeam(forReal) {
        _beam = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        var s = forReal ?
            Sprite.new("[game]/assets/images/projectiles/beam_" + level.toString + ".png") :
            Sprite.new("[game]/assets/images/projectiles/beam_0.png")
        var r = Relation.new(owner)
        s.flags = Render.spriteCenterX | Render.spriteTop // |
        _beam.addComponent(t)
        _beam.addComponent(s)
        _beam.addComponent(r)
    }

    wait { Data.getNumber("Laser Wait") }

    ready { _state == LaserState.idle }
}

import "game" for Game
import "unit" for Unit
