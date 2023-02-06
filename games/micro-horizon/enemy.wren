import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Enemy is Component {
    construct new() {
        super()
        _state = idleState
        _time = 0.0
        _rotationSpeed = 0.0
        _chargePosition = null
        _chargeVelocity = null
        _warning = null
    }

    initialize() {
        super.initialize()
        _transform = owner.getComponent(Transform)
        _body = owner.getComponent(Body)

        // Create the shield ring
        _shields = []
        var t = 0.0
        var dt = Math.pi / 3.0                
        var R = Data.getNumber("Shield Orbit Radius")
        for(i in 0...6) {
            var pos = Vec2.new(t.sin * R, t.cos * R)
            var sh = Create.shield(owner, pos)
            _shields.add(sh)
            t = t + dt
        }

        // Crate the missiles 
        // _missiles
        t = dt / 2.0
        R = Data.getNumber("Missile Orbit Radius")
        for(i in 0...6) {
            var pos = Vec2.new(t.sin * R, t.cos * R)
            var sh = Create.missile(owner, pos)
            t = t + dt
        }

    }    
    
    update(dt) {
        _time = _time + dt
        
        if(_state == idleState) {
            idleUpdate(dt)
        } else if(_state == shieldState) {
            shieldUpdate(dt)
        } else if(_state == chargeWrnState) {
            chargeWrnUpdate(dt)
        } else if(_state == chargeState) {
            chargeUpdate(dt)
        } else if(_state == shootState) {

        } else if(_state == homingState) {

        } else if(_state == homingWrnState) {

        }   

        checkShield()
        
        _transform.rotation = _transform.rotation + _rotationSpeed * dt
    }

    idleUpdate(dt) {
        if(_time > 1.5) {
            _state = shieldState
            _time = 0.0
        }
        setVelocity(Vec2.new(0.0, 0.0), dt)
        setRotationSpeed(1.0, dt)
    }

    chargeWrnUpdate(dt) {
        if(_time > 0.6) {
            _state = chargeState
            _time = 0.0
            _warning.delete()
        }
        setRotationSpeed(1.0, dt)
    }

    chargeUpdate(dt) {
        if((_chargePosition - _transform.position).magnitude < 10) {
            _body.velocity = Vec2.new(0.0, 0.0) 
            _time = 0.0
            _state = idleState 
        }
        setVelocity(_chargeVelocity, dt)
        setRotationSpeed(1.0, dt)
    }

    shieldUpdate(dt) {
        var pt = Game.player.getComponent(Transform)
        var dir = pt.position - _transform.position
        dir.normalise
        setVelocity(dir * 0.2, dt)
        setRotationSpeed(6.0, dt)

        if(_time > 2.0) {
            charge()
        }
    }

    homingWrnUpdate(dt) {
        if(_time > 0.6) {
            _state = chargeState
            _time = 0.0
            _warning.delete()
        }
        setRotationSpeed(1.0, dt)
    }

    homingUpdate(dt) {
        if(_time > 2.0) {
            _state = idleState
            _time = 0.0

            // TODO: 
        }
        setRotationSpeed(1.0, dt)
    }

    charge() {
        _chargePosition = Game.player.getComponent(Transform).position
        var dir = _chargePosition - _transform.position
        dir.normalise
        _chargeVelocity = dir * 2.5
        _time = 0.0
        _state = chargeWrnState
        _warning = Create.warning(_chargePosition)
    }

    launch() {

    }

    checkShield() {
        if(_state == homingWrnState || _state == homingState) {
            return false
        }        
        var deleted = 0
        for(s in _shields) {
            if(s.deleted) {
                deleted = deleted + 1
            }
        }
        if(deleted == _shields.count) {
            _state = homingWrnState
            System.print("Homing")
        }
    } 

    setVelocity(vel, dt) {
        _body.velocity = Math.damp(_body.velocity, vel, 10.0, dt)
    }

    setRotationSpeed(rotSpeed, dt) {
        _rotationSpeed = Math.damp(_rotationSpeed, rotSpeed, 10.0, dt)
    }
    
    // maxHealth { _maxHealth }
    // health { _health }

    // States
    shieldState { 1 }
    chargeState { 2 }
    shootState  { 3 }
    homingState { 4 }
    idleState   { 5 }
    chargeWrnState { 6 }
    homingWrnState { 7 }
}

import "tags" for Team, Tag
import "bullets" for Bullet, Missile
import "debug" for DebugColor
import "random" for Random
import "components" for SlowRelation
import "game" for Game
import "unit" for Unit
import "create" for Create
