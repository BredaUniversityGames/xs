import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Part is Component {
    construct new(follow, dist) {
        super()
        _follow = follow
        _dist = dist
    }

    initialize() {
        _transform = owner.getComponent(Transform)
        _followTransform = _follow.getComponent(Transform)
    }

    update(dt) {
        var dir = _followTransform.position - _transform.position
        if((dir.magnitude - _dist).abs > 2) {
            _transform.rotation = dir.atan2
            var pos = _followTransform.position - dir.normalise * _dist
            _transform.position = Math.damp(_transform.position, pos, 25, dt)
        }
    }
}

class Foot is Component {
    construct new(parent, offset) {
        super()
        _parent = parent
        _offset = offset
        _toPos = null
    }

    initialize() {
        _transform = owner.getComponent(Transform)
    }

    update(dt) {
        var pt = _parent.getComponent(Transform)
        var offset = _offset
        if(pt.rotation != 0.0) {
            offset = _offset.rotated(pt.rotation)        
        }

        var pos = pt.position + offset 

        if(_toPos == null) {
            _toPos = pos
        } else {
            _transform.position = Math.damp(_transform.position, _toPos, 10.0, dt)
        }

        var d = pos - _transform.position
        if(d.magnitude > 50) {
            _toPos = pos
        }

        if(_parent.deleted) {
            owner.delete()
        }
    }
}


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


        var sizes = [30, 30, 20, 15, 10]
        var distances = [43, 64, 54, 38, 28, 25]
        _parts = [owner]
        for(i in 1...6) {
            var pred = _parts[i-1]
            var size = sizes[i-1]
            var dist = distances[i-1]
            var sh = Create.part(pred, dist, size)
            _parts.add(sh)
        }

        var spread = 40
        var gait = 18
        Create.foot(_parts[1], Vec2.new(gait, spread))
        Create.foot(_parts[1], Vec2.new(gait, -spread))
        Create.foot(_parts[2], Vec2.new(-gait, spread))
        Create.foot(_parts[2], Vec2.new(-gait, -spread))
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

        for(i in 1..._parts.count) {
            var p0 = _parts[i]
            var t0 = p0.getComponent(Transform)
            var b0 = p0.getComponent(Body)
            for(j in 0...i) {
                var p1 = _parts[j]
                var t1 = p1.getComponent(Transform)
                var b1 = p1.getComponent(Body)

                var d01 = t0.position - t1.position
                if(d01.magnitude > 0) {
                    var dff = d01.magnitude - (b0.size + b1.size)
                    // System.print("dff: %(dff)")
                    if(dff <= 0) {
                        //System.print("Overlap")
                        t0.position = t0.position - d01.normalise * dff * 0.5
                        continue
                    }
                }
            }
        }
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
        setVelocity(dir * 1.2, dt)
        // setRotationSpeed(6.0, dt)

        if(_time > 200000.0) {
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
        /*
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
        */
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
