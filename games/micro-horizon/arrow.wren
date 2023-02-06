import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Arrow is Component {
    construct new() {
        super()
        _damage = Data.getNumber("Arrow HP")
        _trigger = 0.0
        _ready= false
        _time = 0.0
    }

    initialize() {
        _relation = owner.getComponent(Relation)
        _transform = owner.getComponent(Transform)
    }

    update(dt) {
        _time = _time + dt
        var trig = (Input.getAxis(5) + 1.0) / 2.0
        if(trig == 0.0 /*&& _time > 1.0 */) {
            _ready = true
        }

        if(!_ready) {
            return
        }

        var t = _transform
        var w = Data.getNumber("Width", Data.system) * 0.55
        var h = Data.getNumber("Height", Data.system) * 0.55

        if (t.position.x < -w) {
            owner.delete()
        } else if (t.position.x > w) {
            owner.delete()
        }
        
        if (t.position.y < -h) {
            owner.delete()
        } else if (t.position.y > h) {
            owner.delete()
        }

        _relation.offset = Vec2.new(12 - trig * 12, 0.0)
        var dtrig = _trigger - trig
        _trigger = trig

        if(dtrig > 0.1) {
            System.print("shoot")
            _trigger = 0.0
            shoot()
        }
    }

    shoot() {
        _ready = false
        var pt = _relation.parent.getComponent(Transform)
        var dir = Vec2.new(500.0, 0.0)
        dir = dir.rotated(pt.rotation)

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

    damage { _damage }
}

import "game" for Game
import "unit" for Unit
import "tags" for Team, Tag
import "debug" for DebugColor
import "create" for Create
import "components" for SlowRelation, TurnToVelocity, DeleteOffBounds