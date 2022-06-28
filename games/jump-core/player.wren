import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "globals" for Globals

class Player is Component {
    construct new() {
        super()
        _shootTime = 0
        _drones = []
    }

    update(dt) {    
        // Keep in bounds
        var t = owner.getComponent(Transform)
        var h = Configuration.height * 0.5
        var w = Configuration.width * 0.5
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

        _shootTime = _shootTime + dt
        if((Input.getButton(0) || Input.getKeyOnce(Input.keySpace)) && _shootTime > 0.1) {
            Game.createBullet(owner, Globals.PlayerBulletSpeed, Globals.PlayerBulletDamage)
            for(d in _drones) {
                if(!d.deleted) {
                    Game.createBullet(d, Globals.PlayerBulletSpeed, Globals.PlayerBulletDamage)
                }
            }
            _shootTime = 0
        }

        var b = owner.getComponent(Body)
        var vel = Vec2.new(Input.getAxis(0), -Input.getAxis(1))
        if(Input.getKey(Input.keyUp)) {
            vel.y = 1.0
        }
        if(Input.getKey(Input.keyDown)) {
            vel.y = -1.0
        }
        if(Input.getKey(Input.keyRight)) {
            vel.x = 1.0
        }
        if(Input.getKey(Input.keyLeft)) {
            vel.x = -1.0
        }

        var s = owner.getComponent(GridSprite)
        if(vel.y > 0.5) {
            s.idx = 2
        } else if(vel.y > 0.2) {
            s.idx = 1
        } else if(vel.y < -0.5) {
            s.idx = 4
        } else if(vel.y < -0.2) {
            s.idx = 3
        } else {
            s.idx = 0
        }
        
        var speed = Globals.PlayerSpeed
        if(vel.magnitude > Globals.PlayerInputDeadZone) {            
            vel = vel * speed
        } else {
            vel = vel * 0
        }
        b.velocity = vel        
    }
}

import "game" for Game
