import "xs" for Data, Render
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Sprite, Relation
import "xs_math" for Vec2, Color
import "weapons/bosspart" for BossPart

/*
class Beam is Component {
    construct new(level) {
        super()
        _time = 0
    }

    initialize() {
        _sprite = owner.getComponentSuper(Sprite)
    }

    update(dt) {        
        _time = _time + dt
        if()
        var val = _time.sin
        var color = Color.new(255,255,255, 255 * val)        
        _sprite.mul = color.toNum

        if(val < 0.0) {
            owner.delete()
        }
    }
}
*/

class LaserState {
    static idle     { 0 }
    static prepare  { 1 }
    static shoot    { 2 }
    static cooldown { 3 }
}

class Laser is BossPart {
    construct new(level) {
        super()
        _level = level
        _time = 0     
        _beam = null           
        _state = LaserState.idle
    }

        /*
    initialize() {
        super.initialize()
    }
    */

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
            // Do some damage here
            if(_time > 1.0) {
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
            Sprite.new("[game]/assets/images/projectiles/beam_" + _level.toString + ".png") :
            Sprite.new("[game]/assets/images/projectiles/beam_0.png")
        var r = Relation.new(owner)
        s.flags = Render.spriteCenterX | Render.spriteTop // |
        _beam.addComponent(t)
        _beam.addComponent(s)
        _beam.addComponent(r)
    }

    ready { _state == LaserState.idle }
}
