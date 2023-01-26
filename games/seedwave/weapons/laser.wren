import "xs" for Data, Render
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Sprite, Relation
import "xs_math" for Vec2, Color
import "weapons/bosspart" for BossPart

class Beam is Component {
    construct new() {
        super()
        _time = 0
    }

    initialize() {
        _sprite = owner.getComponentSuper(Sprite)
    }

    update(dt) {
        _time = _time + dt * 3
        var val = _time.sin
        var color = Color.new(255,255,255, 255 * val)        
        _sprite.mul = color.toNum

        if(val < 0.0) {
            owner.delete()
        }
    }
}


class Laser is BossPart {
    construct new(level) {
        super()
        _level = level
        _time = 0     
        _beam = null           
    }

    /*
    initialize() {
        super.initialize()
        {

        }
    }

    update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Laser Shoot Time")) {
            _time = 0
        }
        super.update(dt)
    }
    */
    
    shoot() {
        _beam = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        var s = Sprite.new("[game]/assets/images/projectiles/beam_" + _level.toString + ".png")
        var r = Relation.new(owner)
        var b = Beam.new()
        s.flags = Render.spriteCenterX | Render.spriteTop // |
        _beam.addComponent(t)
        _beam.addComponent(s)
        _beam.addComponent(r)
        _beam.addComponent(b)
    }

    active { super.active && (_beam == null || _beam.deleted) }

    // makeBeam() {}
}
