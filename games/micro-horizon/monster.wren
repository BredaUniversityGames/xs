import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Geom
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

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


class Monster is Component {

    construct new() {
        super()
        _state = Monster.idleState
        _time = 0.0
        _rotationSpeed = 0.0
        _chargePosition = null
        _chargeVelocity = null
        _warning = null
    }

    static getPartSize(i) { Data.getNumber("Part Size %(i)") }

    initialize() {
        super.initialize()
        _transform = owner.getComponent(Transform)
        _body = owner.getComponent(Body)
        
        _parts_old = [owner]
        _parts = [owner.getComponent(TNB)]

        var n = 7
        for(i in 1...n) {
            var size = Monster.getPartSize(i)
            var part = Create.part(size, i)
            _parts_old.add(part)
            var tnb = part.getComponent(TNB)
            _parts.add(tnb)            
        }

        var spread = 40
        var gait = 18
        Create.foot(_parts[1].owner, Vec2.new(gait, spread))
        Create.foot(_parts[1].owner, Vec2.new(gait, -spread))
        Create.foot(_parts[2].owner, Vec2.new(-gait, spread))
        Create.foot(_parts[2].owner, Vec2.new(-gait, -spread))

        spread = 12
        gait = 8
        Create.hitbox(_parts[0].owner, Vec2.new(12.0, 0.0), 15)
        Create.hitbox(_parts[1].owner, Vec2.new(-gait, spread), 25)
        Create.hitbox(_parts[1].owner, Vec2.new(-gait, -spread), 25)
        Create.hitbox(_parts[2].owner, Vec2.new(-gait, spread), 25)
        Create.hitbox(_parts[2].owner, Vec2.new(-gait, -spread), 25)

        _laser = Create.laser(_parts_old[1], 20)
    }    
    
    update(dt) {

        // Some warmup time to initialize
        if(!_parts[_parts.count - 1].initialized_) {
            return
        }


        _time = _time + dt
        
        if(_state == Monster.idleState) {
            idleUpdate(dt)
        } else if(_state == Monster.moveState) {
            moveUpdate(dt)
        } else if(_state == Monster.chargeWrnState) {
            chargeWrnUpdate(dt)
        } else if(_state == Monster.chargeState) {
            chargeUpdate(dt)
        }
        
        fakysics(dt)
    }

    // This is the entire animation rig
    fakysics(dt) {
        // Make things follow
        for(i in 1..._parts.count) {
            var p0 = _parts[i-1]
            var p1 = _parts[i]
            var dist = p1.size + p0.size + 5

            // Keep distance (harsh)
            var dir = p0.position - p1.position
            p1.rotation = dir.atan2
            if((dir.magnitude - dist).abs > 3) {   // TODO                
                var pos = p0.position - dir.normal * dist
                p1.position = Math.damp(p1.position, pos, 25, dt) //TODO
            }

            // Reduce sharp turns
            {   
                var offset = Vec2.new (-dist, 0.0)
                offset = offset.rotated(p0.rotation)
                var pos = p0.position + offset 

                var desDir = offset - p1.position
                dir = dir.normal
                desDir = desDir.normal
                var a = dir.dot(desDir)
                a = 1.0 - a
                p1.position = Math.damp(p1.position, pos, 1.2 * a, dt) //TODO
            }
        }

        // Resolve pieces overlap
        for(i in 1..._parts.count) {
            var p0 = _parts[i]
            for(j in 0...i) {
                var p1 = _parts[j]

                var d01 = p0.position - p1.position
                if(d01.magnitude > 0) {
                    var dff = d01.magnitude - (p0.size + p1.size)
                    // System.print("dff: %(dff)")
                    if(dff <= 0) {
                        //System.print("Overlap")
                        p0.position = p0.position - d01.normal * dff * 0.75
                        // continue
                    }
                }
            }
        }

        // Resolve tail overlap
        for(i in 1...(_parts.count - 1)) {
            var curr = _parts[i]
            var next = _parts[i+1]
            for(j in 0...i) {
                var prev = _parts[j]
                var dist = Geom.distanceSegmentToPoint(curr.position, next.position, prev.position)

                if(dist < prev.size) {
                    var offset = prev.size - dist
                    // System.print("offset: %(offset)")
                    var middle = (curr.position + next.position) * 0.5
                    var normal = middle - prev.position
                    normal = normal.normal

                    curr.position = curr.position + normal * offset
                    next.position = next.position + normal * offset                    
                }
            }
        }
    }

    idleUpdate(dt) {
        if(_time > 1.0) {   // TODO
            _state = Monster.moveState
            _time = 0.0
        }
        setVelocity(Vec2.new(0.0, 0.0), dt)
    }

    chargeWrnUpdate(dt) {
        if(_time > 0.6) {   // TODO
            _state = Monster.chargeState
            _time = 0.0
            _warning.delete()
        }
    }

    chargeUpdate(dt) {
        if((_chargePosition - _transform.position).magnitude < 10) {    // TODO
            _body.velocity = Vec2.new(0.0, 0.0) 
            _time = 0.0
            _state = Monster.idleState 
        }
        setVelocity(_chargeVelocity, dt)
    }

    moveUpdate(dt) {
        var pt = Game.player.getComponent(Transform)
        var dir = pt.position - _transform.position

        var alpha = dir.atan2
        _transform.rotation = alpha

        if(dir.magnitude > 150.0) {     // TODO
            dir.normal
            setVelocity(dir * 0.6, dt)  // TODO
            if(_time > 3.0) {           // TODO
                launch()
            }
        } else {
            setVelocity(dir * 0.0, dt)  // TODO
            if(_time > 3.0) {           // TODO
                charge()
            }
        }

        if(_time > 6.0) {   // TODO
            charge()
        }
    }

    charge() {
        _chargePosition = Game.player.getComponent(Transform).position
        var dir = _chargePosition - _transform.position
        dir = dir.normal
        _chargeVelocity = dir * 900.5   // TODO
        _time = 0.0
        _state = Monster.chargeWrnState
        _warning = Create.warning(_chargePosition)
        _chargePosition = _chargePosition + dir * 80 // TODO
    }

    setVelocity(vel, dt) {
        _body.velocity = Math.damp(_body.velocity, vel, 10.0, dt) // TODO
    }

    launch() {
        var choice = Game.random.int(0, 2)

        if(choice == 0) {
            for(i in 0...3) {
                Create.missile(_parts[2].owner, 200, 2)   // TODO                
                _state = Monster.idleState
                _time = 0.0
            }
        } else if(choice == 1) {
            _laser.getComponent(Laser).zing()
            _state = Monster.idleState
            _time = -1.0
        }
    }
    
    // maxHealth { _maxHealth }
    // health { _health }

    // States
    static idleState       { 1 }
    static moveState       { 2 }
    static chargeState     { 3 }
    static chargeWrnState  { 4 }
    static homingWrnState  { 5 }
}

import "tags" for Team, Tag
import "bullets" for Bullet, Missile, Laser
import "debug" for DebugColor
import "random" for Random
import "components" for SlowRelation, TNB
import "game" for Game
import "unit" for Unit
import "create" for Create
