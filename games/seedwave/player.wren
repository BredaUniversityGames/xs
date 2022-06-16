import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Player is Component {

    construct new() {
        super()
        _shootTime = 0
        _animFrame = 0
        _frame = 0
    }

    update(dt) {
        // Get input        

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

        /*
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
        */

        var speed = Registry.getNumber("Player Speed")
        if(Input.getButton(1)) {
            speed = Registry.getNumber("Player Speed When Aiming")
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
        
        _frame = _frame + 1
        if(_frame > 4) {
            _frame = 0
            _animFrame = (_animFrame + 1) % 4
        }


        if(vel.x > 0.5) {
            s.idx = 16
        } else if(vel.x > 0.35) {
            s.idx = 12
        } else if(vel.x < -0.5) {
            s.idx = 8
        } else if(vel.x < -0.35) {
            s.idx = 4
        } else {
            s.idx = 0
        }
        s.idx = s.idx + _animFrame
        
        if(vel.magnitude > Registry.getNumber("Player Input Dead Zone")) {            
            vel = vel * speed
        } else {
            vel = vel * 0
        }
        b.velocity = vel        
    }
}
