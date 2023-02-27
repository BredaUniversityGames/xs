import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Boss is Component {
        static init() {
        __random = Random.new()
        __offsets = []

        __offsets.add(Vec2.new(0, 0))
        __offsets.add(Vec2.new(0, 1))
        __offsets.add(Vec2.new(0, -1))

        __offsets.add(Vec2.new(1, 0))
        __offsets.add(Vec2.new(1, 1))
        __offsets.add(Vec2.new(1, -1))
        __offsets.add(Vec2.new(1, 2))
        __offsets.add(Vec2.new(1, -2))

        __offsets.add(Vec2.new(2, 0))
        __offsets.add(Vec2.new(2, 1))
        __offsets.add(Vec2.new(2, -1))
        __offsets.add(Vec2.new(2, 2))
        __offsets.add(Vec2.new(2, -2))

        __offsets.add(Vec2.new(3, 0))
        __offsets.add(Vec2.new(3, 1))
        __offsets.add(Vec2.new(3, -1))
        __offsets.add(Vec2.new(3, 2))
        __offsets.add(Vec2.new(3, -2))
        __offsets.add(Vec2.new(3, 3))
        __offsets.add(Vec2.new(3, -3))

        __offsets.add(Vec2.new(4, 0))
        __offsets.add(Vec2.new(4, 1))
        __offsets.add(Vec2.new(4, -1))
        __offsets.add(Vec2.new(4, 2))
        __offsets.add(Vec2.new(4, -2))
        __offsets.add(Vec2.new(4, 3))
        __offsets.add(Vec2.new(4, -3))


        __offsets.add(Vec2.new(5, 0))
        __offsets.add(Vec2.new(5, 1))
        __offsets.add(Vec2.new(5, -1))
        __offsets.add(Vec2.new(5, 2))
        __offsets.add(Vec2.new(5, -2))
        __offsets.add(Vec2.new(5, 3))
        __offsets.add(Vec2.new(5, -3))


        __offsets.add(Vec2.new(6, 0))
        __offsets.add(Vec2.new(6, 1))
        __offsets.add(Vec2.new(6, -1))
        __offsets.add(Vec2.new(6, 2))
        __offsets.add(Vec2.new(6, -2))
        __offsets.add(Vec2.new(6, 3))
        __offsets.add(Vec2.new(6, -3))


        __offsets.add(Vec2.new(7, 0))
        __offsets.add(Vec2.new(7, 1))
        __offsets.add(Vec2.new(7, -1))
        __offsets.add(Vec2.new(7, 2))
        __offsets.add(Vec2.new(7, -2))
        __offsets.add(Vec2.new(7, 3))
        __offsets.add(Vec2.new(7, -3))

    }    

    static part(boss, type, level, position) {
        var l = __random.float(
            Data.getNumber("Part delay lambda from"),
            Data.getNumber("Part delay lambda to"))

        var size = Boss.getPartSize(level)
        var health = 0
        if(level == 1) {
            health = Data.getNumber("Part Health 1")
        } else if (level == 2) {
            health = Data.getNumber("Part Health 2")
        } else if (level == 3) {
            health = Data.getNumber("Part Health 3")
        }

        var part = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        var b = Body.new(size, Vec2.new(0, 0))
        var u = Unit.new(Team.Computer, health, false)
        var c = DebugColor.new(0xFFFFFFFF)
        var r = SlowRelation.new(boss, l)
        r.offset = position
        part.addComponent(t)
        //part.addComponent(bs)
        part.addComponent(b)
        part.addComponent(u)        
        part.addComponent(r)
        part.name = "Part"
        part.tag = (Tag.Computer | Tag.Unit)

        var s =  GridSprite.new("[game]/assets/images/ships/parts_" + level.toString + ".png", 8, 1)
        if(type == "C") {                                 //  Cannon
            s.idx = 0
            c = DebugColor.new(0xFFA8D3FF)
            var w = Cannon.new(level)
            part.addComponent(w)
        } else if (type == "L") {                           // Laser
            s.idx = 1
            var l = Laser.new(level)
            part.addComponent(l)
            c = DebugColor.new(0x9896FFFF)
        } else if(type == "M") {                            // Missiles
            s.idx = 2
            c = DebugColor.new(0xFDFFC1FF)
            var w = Missiles.new(level)
            part.addComponent(w)
        } else if(type == "D") {                            // Deflect
            part.tag = (part.tag | Tag.Deflect)             // |
            s.idx = 3
            var d = Deflect.new(level)
            part.addComponent(d)
            c = DebugColor.new(0xFFB599FF)
        } else if(type == "N") {                            // Needler        
            s.idx = 4            
            c = DebugColor.new(0xFFB599FF)
            var n = Needler.new(level)
            part.addComponent(n)
        } else if(type == "E") {                            // EMP
            s.idx = 5
            c = DebugColor.new(0xFFB599FF)
            var e = EMP.new(level)
            part.addComponent(e)
        } else if(type == "R") {                            // Helpers
            s.idx = 6
            c = DebugColor.new(0xFFB599FF)
            var d = Drones.new(level)
            part.addComponent(d)
        } else if(type == "V") {                            // Vulcan
            s.idx = 7
            c = DebugColor.new(0xFFB599FF)
            var v = Vulcan.new(level)
            part.addComponent(v)
        }
        s.layer = 1.0
        s.flags = Render.spriteCenter
        part.addComponent(c)
        part.addComponent(s)
        return part
    }

    static getPartSize(level) {
        if(level == 1) {
            return Data.getNumber("Part Size 1")
        } else if (level == 2) {
            return Data.getNumber("Part Size 2")
        } else if (level == 3) {
            return Data.getNumber("Part Size 3")
        }
        return 0
    }

    static create(dna) {
        System.print("Creating boss with dna %(dna)")
        var ship = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)        
        var v = Vec2.new(0, 0)
        var b = Body.new(Data.getNumber("Core Size"), v)
        var u = Unit.new(Team.Computer, Data.getNumber("Core Health"), true)
        var c = DebugColor.new(0xFF2222FF)
        var s = Sprite.new("[game]/assets/images/ships/core.png")
        u.multiplier = 0.1
        s.layer = 1.0
        s.flags = Render.spriteCenter
        ship.addComponent(t)        
        ship.addComponent(b)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(s)
        ship.name = "Boss"
        ship.tag = (Tag.Computer | Tag.Unit)        // |||
        var type = null
        var flip = false
        var idx = 0
        var offsets = [Vec2.new(0,0)]
        var radii = [Data.getNumber("Core Size")]        
        var pairs = []
        var maxOrbit = 0        
        for(i in dna) {
            var level = Num.fromString(i)
            if(level != null && level != 0 && type != null) {
                var pos = Vec2.new(__offsets[idx].x, __offsets[idx].y)                
                pos = getOffset(pos.x, pos.y, maxOrbit)
                var rad = getPartSize(level)

                var guard = 0
                while(true) {
                    var overlap = 0
                    for(j in 0...offsets.count) {
                        var op = offsets[j]
                        var or = radii[j]
                        var to = pos - op
                        var mg = to.magnitude                
                        var df = (or + rad) - mg 
                        if(df > 0) {
                            var tn = to.normal
                            pos = pos + (tn * df)
                            overlap = df
                        }
                    }
                    if(overlap == 0) {
                        break
                    }

                    guard = guard + 1
                    if(guard >= 1000) {
                        break
                    }
                }

                if(pos.magnitude > maxOrbit) {
                    maxOrbit = pos.magnitude
                }

                var posL = Vec2.new(-pos.x, pos.y)
                var partR = part(ship, type, level, pos)
                var partL = part(ship, type, level, posL)                
                pairs.add( [partR.getComponentSuper(BossPart), partL.getComponentSuper(BossPart)] )
                level = null
                type = null
                idx = idx + 1
                offsets.add(pos)
                radii.add(rad)                
            } else if(i == "S") {
                idx = idx + 1
            } else {
                type = i
            }
        }

        for(pr in  pairs) {
            for(p in pr) {
                var r = p.owner.getComponent(SlowRelation)
                var s = p.owner.getComponentSuper(Sprite)
                s.layer = 1 + r.offset.y * 0.01
            }
        }


        var bs = Boss.new(pairs)
        ship.addComponent(bs) 
        return ship
    }    

    static getOffset(orbit, slot, maxOrbit) {
        var dr = Data.getNumber("Boss Orbit Radius")
        var rad = orbit * dr + Data.getNumber("Boss Core 1 Radius")        
        if(maxOrbit > rad) {
            rad = maxOrbit
        }
        var dl = Data.getNumber("Boss Orbit Angle") / rad
        var a = dl * slot        
        var pos = Vec2.new(a.cos * rad, a.sin * rad)
        return pos
    }

    static debugRender(position) {
        for(off in __offsets) {
            var pos = getOffset(off.x, off.y, 0.0) + position
            Render.disk(pos.x, pos.y, 3.0, 12)
        }
    }

    construct new(pairs) {
        super()
        _time = 0
        _sinTime = 0
        _pairs = pairs
        _wait = 0.5           
        _health = 0.0        
    }

    initialize() {
        _unit = owner.getComponent(Unit)
        _maxHealth = 0.0 // _unit.maxHealth
        for(pr in  _pairs) {
            for(p in pr) {
                var u = p.owner.getComponent(Unit)
                _maxHealth = _maxHealth + u.maxHealth
            }
        }
    }    
    
    update(dt) {
        var ot = owner.getComponent(Transform)
        var ob = owner.getComponent(Body)
        var pt = Game.player.getComponent(Transform)

        var dir = pt.position - ot.position        
        dir.y = 0
        dir.normal
        ob.velocity = dir * 1.5

        _sinTime = _sinTime + dt 
        ot.position.y = _sinTime.cos * 20 + 60        

        _time = _time + dt

        if(_time > _wait) {
            _time = 0
            var i = Game.random.int(-1, _pairs.count) 
            if(i != -1) {   
                var pair = _pairs[i]
                _wait = 0.0
                for(i in 0..1) {
                    var part = pair[i]            
                    if(!part.destroyed && part.ready) {
                        pair[i].shoot()
                        _wait = _wait + part.wait
                    }
                }
            } else {
                shoot()
                _wait = _wait + 0.15   
            }
        }

        _health = 0.0
        for(pr in  _pairs) {
            for(p in pr) {
                var u = p.owner.getComponent(Unit)
                if(u != null) {
                    var maxHealth = _maxHealth + u.maxHealth
                    _health = _health + u.health
                }
            }
        }
        var u = owner.getComponent(Unit)
        if(_health <= 0.0) {
            u.multiplier = 1
        } else {
            u.multiplier = 0.2
        }
        _health = _health + u.health
    }

    shoot() {
        Create.enemyBullet(
            owner,
            -Data.getNumber("Cannon Round Speed"),
            Data.getNumber("Cannon Damage"),
            Vec2.new())
    }

    maxHealth { _maxHealth }
    health { _health }

    debugRender() {
        var pos = owner.getComponent(Transform).position
        Boss.debugRender(pos)
    }
}

import "tags" for Team, Tag
import "bullets" for Bullet, Missile
import "debug" for DebugColor
import "random" for Random
import "components" for SlowRelation
import "weapons/bosspart" for BossPart
import "weapons/laser" for Laser
import "weapons/cannon" for Cannon
import "weapons/missiles" for Missiles
import "weapons/deflect" for Deflect
import "weapons/needler" for Needler
import "weapons/vulcan" for Vulcan
import "weapons/emp" for EMP
import "weapons/drones" for Drones
import "game" for Game
import "unit" for Unit
import "create" for Create
