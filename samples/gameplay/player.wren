import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
// import "unit" for Unit
// import "tags" for Team, Tag
// import "bullets" for Bullet
// import "debug" for DebugColor
// import "components" for SlowRelation
import "random" for Random

class Player is Component {
    construct new() {
        super()
        _time = 0        
    }

    initialize() {
        _body = owner.getComponent(Body)
        _transform = owner.getComponent(Transform)
        // var aim = Create.aim(owner)
    }

    update(dt) {
        /*
        if(_state == stateNormal) {
            _cooldown = _cooldown - dt
            move(dt)
            checkDodge()
            keepInBounds()
        } else if(_state == stateDodge) {
            dodge(dt)
        }
        */
        move(dt)
    }

    move(dt) {                
        // Translation
        var aim = (Input.getAxis(5) + 1.0) / 2.0
        var normalSpeed = Data.getNumber("Player Speed")
        var aimSpeed = Data.getNumber("Player Speed Aim")
        var speed = Math.lerp(normalSpeed, aimSpeed, aim)
        var vel = Vec2.new(Input.getAxis(0), -Input.getAxis(1))
        if(vel.magnitude > Data.getNumber("Player Input Dead Zone")) {            
            vel = vel * speed
        }
        var posEase = Data.getNumber("Player Position Easing")
        _body.velocity = Math.damp(_body.velocity, vel, posEase, dt)

        // Rotation
        var face = Vec2.new(Input.getAxis(2), -Input.getAxis(3))
        if(face.magnitude > Data.getNumber("Player Input Dead Zone")) {
            var a = face.atan2
            _transform.rotation = a
        }
    }

    keepInBounds() {
        var t = _transform
        var h = Data.getNumber("Height", Data.system) * 0.5
        var w = Data.getNumber("Width", Data.system) * 0.5
        if (t.position.x < -w) {
            t.position.x = -w
        } else if (t.position.x > w) {
            t.position.x = w
        }
        if (t.position.y < -h) {
            t.position.y = -h
        } else if (t.position.y > h) {
            t.position.y = h
        }
    }

    toString { "[Player]" }
}

import "create" for Create
