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
    }

    update(dt) {
        move(dt)
        shoot(dt)
        keepInBounds()
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

        // Rotation - support both gamepad right stick and mouse
        var face = Vec2.new(Input.getAxis(2), -Input.getAxis(3))
        if (face.magnitude > Data.getNumber("Player Input Dead Zone")) {
            // Gamepad aim
            var a = face.atan2
            _transform.rotation = a
        } else {
            // Mouse aim - point toward mouse cursor
            var mousePos = Vec2.new(Input.getMouseX(), Input.getMouseY())
            var screenCenter = Vec2.new(
                Data.getNumber("Width", Data.system) * 0.5, 
                Data.getNumber("Height", Data.system) * 0.5
            )
            var worldMouse = mousePos - screenCenter
            var toMouse = worldMouse - _transform.position
            if (toMouse.magnitude > 10) {
                _transform.rotation = toMouse.atan2
            }
        }
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

    toString { "[Player]" }
}

import "create" for Create
