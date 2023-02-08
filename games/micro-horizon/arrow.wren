import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "random" for Random

// Arrow being pulled in the bow
// When flying it changes components 
class Arrow is Component {
    construct new() {
        super()
        _damage = Data.getNumber("Arrow HP")
        _trigger = 0.0
        _ready= false
        _time = 0.0
        _pull = 0.0
    }

    initialize() {
        owner.tag = Tag.Bow
        _relation = owner.getComponent(Relation)
        _transform = owner.getComponent(Transform)
    }

    update(dt) {
        _time = _time + dt
        var trig = (Input.getAxis(5) + 1.0) / 2.0
        if(trig == 0.0) {
            _ready = true
            _pull = 0.0
        }

        if(!_ready) {
            return
        }

        var t = _transform    
        _pull = Math.damp(_pull, trig, 5.0, dt)
        _relation.offset = Vec2.new(12 - _pull * 12, 0.0)
        var dtrig = _trigger - trig
        _trigger = trig

        if(dtrig > 0.1) {
            _trigger = 0.0
            shoot()
        }
    }

    shoot() {
        _ready = false
        var pt = _relation.parent.getComponent(Transform)
        var dir = Vec2.new(500.0, 0.0)
        var spread = (1.0 - _pull) * Math.pi / 3.0
        System.print("Pull: %(_pull) spread %(spread)")
        var randrot = Game.random.float(-spread, spread)
        dir = dir.rotated(pt.rotation + randrot)

        var b = Body.new(2, dir)
        owner.addComponent(b)

        var a = owner.getComponent(AnimatedSprite)
        a.playAnimation("play")

        var t = TurnToVelocity.new()
        owner.addComponent(t)

        var d = DeleteOffBounds.new()
        owner.addComponent(d)

        owner.deleteComponent(Arrow)
        owner.deleteComponent(Relation)
        owner.tag = (Tag.Player | Tag.Bullet)
        Create.arrow(_relation.parent)
    }

    pull { _pull }
}

import "game" for Game
import "unit" for Unit
import "tags" for Team, Tag
import "debug" for DebugColor
import "create" for Create
import "components" for SlowRelation, TurnToVelocity, DeleteOffBounds