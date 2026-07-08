import "xs/core" for Input, Render, Data
import "xs/ec"for Entity, Component
import "xs/math"for Math, Bits, Vec2, Color
import "xs/components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "random" for Random

class Player is Component {
    construct new() {
        super()
        _time = 0
        _shootCooldown = 0
    }

    initialize() {
        _body = owner.get(Body)
        _transform = owner.get(Transform)
        _shootInterval = Data.getNumber("Player Shoot Interval")
        _sprite = owner.get(GridSprite)
        _facing = _body.velocity
    }

    update(dt) {
        move(dt)
        shoot(dt)
        keepInBounds()
        updateSprite()
    }

    move(dt) {                
        // Translation - support both gamepad and keyboard (WASD)
        var vel = Vec2.new(0, 0)
        
        // Gamepad input
        var gamepadX = Input.getAxis(0)
        var gamepadY = -Input.getAxis(1)
        if (gamepadX.abs > Data.getNumber("Player Input Dead Zone") || 
            gamepadY.abs > Data.getNumber("Player Input Dead Zone")) {
            vel = Vec2.new(gamepadX, gamepadY)
        } else {
            // Keyboard input (WASD)
            if (Input.getKey(Input.keyA)) {
                vel = vel + Vec2.new(-1, 0)
            }
            if (Input.getKey(Input.keyD)) {
                vel = vel + Vec2.new(1, 0)
            }
            if (Input.getKey(Input.keyW)) {
                vel = vel + Vec2.new(0, 1)
            }
            if (Input.getKey(Input.keyS)) {
                vel = vel + Vec2.new(0, -1)
            }
        }
        
        var normalSpeed = Data.getNumber("Player Speed")
        if (vel.magnitude > Data.getNumber("Player Input Dead Zone")) {            
            vel = vel.normal * normalSpeed
        }
        var posEase = Data.getNumber("Player Position Easing")
        _body.velocity = Math.damp(_body.velocity, vel, posEase, dt)
        
        var damp = Data.getNumber("Player.Rotation Damp")
        _facing = Math.damp(_facing, vel, damp, dt)
        _transform.rotation = _facing.atan2
    }

    shoot(dt) {
        _shootInterval = Data.getNumber("Player Shoot Interval")
        _shootCooldown = _shootCooldown - dt
        
        // Auto-shoot when cooldown is ready
        if (_shootCooldown <= 0) {
            _shootCooldown = _shootInterval
            
            // Shoot in the direction the player is facing
            var direction = Vec2.new(_transform.rotation.cos, _transform.rotation.sin)
            Create.bullet(_transform.position, direction)
        }
    }

    keepInBounds() {
        var t = _transform
        var h = Data.getNumber("World Height") * 0.5
        var w = Data.getNumber("World Width") * 0.5
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

    updateSprite() {
        var maxSpeed = Data.getNumber("Player Speed")
        var normalizedSpeed = _body.velocity.magnitude / maxSpeed

        _sprite.idx = (normalizedSpeed * 16).floor
    }

    toString { "[Player]" }
}

import "create" for Create
