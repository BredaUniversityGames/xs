import "xs" for Data, Render
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Sprite, Relation
import "xs_math" for Vec2
import "weapons/bosspart" for BossPart

/*
class Beam is Component {
    construct new() {

    }
}
*/

class Laser is BossPart {
    construct new() {
        super()
        _time = 0        
    }

    initialize() {
        super.initialize()
        {
            var e = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = Sprite.new("[game]/assets/images/projectiles/beam_m.png")
            var r = Relation.new(owner)
            // s.flags = Render.spriteCenter
            e.addComponent(t)
            e.addComponent(s)
            e.addComponent(r)
        }
    }

    update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Laser Shoot Time")) {
            _time = 0
        }
        super.update(dt)
    }
    
    shoot() {}

    makeBeam() {}
}
