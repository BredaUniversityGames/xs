import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "tags" for Team, Tag
import "bullets" for Bullet, Missile
import "debug" for DebugColor
import "random" for Random
import "components" for SlowRelation

import "weapons/bosspart" for BossPart
import "weapons/laser" for Laser
import "weapons/cannon" for Cannon
import "weapons/missiles" for Missiles

//import "weapons/needler" for Needler

class Boss is Component {
        static init() {
        __random = Random.new()
        __offsets = []
        __dna = ""

        __offsets.add(Vec2.new(0, 0))
        __offsets.add(Vec2.new(0, 1))
        __offsets.add(Vec2.new(0, -1))
        //__offsets.add(Vec2.new(0, 2))
        //__offsets.add(Vec2.new(0, -2))

        __offsets.add(Vec2.new(1, 0))
        __offsets.add(Vec2.new(1, 1))
        __offsets.add(Vec2.new(1, -1))
        __offsets.add(Vec2.new(1, 2))
        __offsets.add(Vec2.new(1, -2))

        __offsets.add(Vec2.new(2, 0))
        __offsets.add(Vec2.new(2, 1))
        __offsets.add(Vec2.new(2, -1))
        __offsets.add(Vec2.new(2, 2))
        __offsets.add(Vec2.new(2, -3))

        __offsets.add(Vec2.new(3, 0))
        __offsets.add(Vec2.new(3, 1))
        __offsets.add(Vec2.new(3, -1))
        __offsets.add(Vec2.new(3, 2))
        __offsets.add(Vec2.new(3, -3))

        __offsets.add(Vec2.new(4, 0))
        __offsets.add(Vec2.new(4, 1))
        __offsets.add(Vec2.new(4, -1))
        __offsets.add(Vec2.new(4, 2))
        __offsets.add(Vec2.new(4, -2))

        __offsets.add(Vec2.new(5, 0))
        __offsets.add(Vec2.new(5, 1))
        __offsets.add(Vec2.new(5, -1))
        __offsets.add(Vec2.new(5, 2))
        __offsets.add(Vec2.new(5, -2))

        __offsets.add(Vec2.new(6, 0))
        __offsets.add(Vec2.new(6, 1))
        __offsets.add(Vec2.new(6, -1))
        __offsets.add(Vec2.new(6, 2))
        __offsets.add(Vec2.new(6, -2))

        __offsets.add(Vec2.new(7, 0))
        __offsets.add(Vec2.new(7, 1))
        __offsets.add(Vec2.new(7, -1))
        __offsets.add(Vec2.new(7, 2))
        __offsets.add(Vec2.new(7, -2))
    }

    static generateDna(size) {
        System.print(size)
        var dna = ""
        var protein = 0
        while(protein <= size) {
            var r = __random.int(-1, 9)
            var l = __random.int(1, 4)
            if(r <= 0) {
                dna = dna + "S"                 // Skip
                continue
            } else if(r == 1) {
                dna = dna + "C" + l.toString    // Canons
            } else if(r == 2) {
                dna = dna + "L" + l.toString    // Lasers
            } else if(r == 3) {
                dna = dna + "M" + l.toString    // Missles
            } else if(r == 4) {
                dna = dna + "D" + l.toString    // Deflect
            } else if(r == 5) {
                dna = dna + "N" + l.toString    // Needler
            } else if(r == 6) {
                dna = dna + "E" + l.toString    // EMP
            } else if(r == 7) {
                dna = dna + "H" + l.toString    // Helpers
            } else if(r == 7) {
                dna = dna + "V" + l.toString    // Vulcan
            } 
            protein = protein + l
        }
        return dna
    }

    static part(boss, type, level, position) {
        var l = __random.float(
            Data.getNumber("Part delay lambda from"),
            Data.getNumber("Part delay lambda to"))

        var size = Boss.getPartSize(level)
        var health = 0
        var str = ""
        if(level == 1) {
            health = Data.getNumber("Part Health 1")
            str = "s"
        } else if (level == 2) {
            health = Data.getNumber("Part Health 2")
            str = "m"
        } else if (level == 3) {
            health = Data.getNumber("Part Health 3")
            str = "l"
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

        var s =  GridSprite.new("[game]/assets/images/ships/parts_" + str + ".png", 8, 1)
        if(type == "C") {                                 //  Cannon
            s.idx = 0
            c = DebugColor.new(0xFFA8D3FF)
            var w = Cannon.new()
            part.addComponent(w)
        } else if (type == "L") {                           // Laser
            s.idx = 1
            var l = Laser.new()
            part.addComponent(l)
            c = DebugColor.new(0x9896FFFF)
        } else if(type == "M") {                            // Missiles
            s.idx = 2
            c = DebugColor.new(0xFDFFC1FF)
            var w = Missiles.new()
            part.addComponent(w)
        } else if(type == "D") {                            // Deflect
            part.tag = (part.tag| Tag.Deflect)              // |
            s.idx = 3
            c = DebugColor.new(0xFFB599FF)
        } else if(type == "N") {                            // Needler
            s.idx = 4
            c = DebugColor.new(0xFFB599FF)
        } else if(type == "E") {                            // EMP
            s.idx = 5
            c = DebugColor.new(0xFFB599FF)
        } else if(type == "H") {                            // Helpers
            s.idx = 6
            c = DebugColor.new(0xFFB599FF)
        } else if(type == "V") {                            // Vulcan
            s.idx = 7
            c = DebugColor.new(0xFFB599FF)
        }
        s.layer = 1.0
        s.flags = Render.spriteCenter
        part.addComponent(c)
        part.addComponent(s)
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
        System.print(dna)
        var ship = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var bs = Boss.new()
        var v = Vec2.new(0, 0)
        var b = Body.new(Data.getNumber("Core Size"), v)
        var u = Unit.new(Team.Computer, Data.getNumber("Core Health"), true)
        var c = DebugColor.new(0xFF2222FF)
        var s = Sprite.new("[game]/assets/images/ships/core.png")
        s.layer = 1.0
        s.flags = Render.spriteCenter
        ship.addComponent(t)
        ship.addComponent(bs)
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
        var maxOrbit = 0        
        for(i in dna) {
            var level = Num.fromString(i)
            if(level != null && type != null) {
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
                            var tn = to.normalise
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
        return ship        
    }

    static randomBoss(size) {        
        var dna = ""
        if(!Data.getString("DNA", Data.debug).isEmpty) {
            dna = Data.getString("DNA", Data.debug)
        } else if(Data.getNumber("Protein", Data.debug) != 0) {
            dna = generateDna(Data.getNumber("Protein", Data.debug))
        } else {
            dna = generateDna(size)
        }
        __dna = dna
        return create(dna)
    }

    static dna { __dna }

    static getOffset(orbit, slot, maxOrbit) {
        var dr = Data.getNumber("Boss Orbit Radius")
        var rad = orbit * dr + Data.getNumber("Boss Core 1 Radius")        
        if(maxOrbit > rad) {
            rad = maxOrbit
        }
        var dl = Data.getNumber("Boss Orbit Angle") / rad
        var a = dl * slot        
        var pos = Vec2.new(a.cos * rad, a.sin * rad)
        //System.print(pos)
        return pos
    }


    construct new() {
        super()
        _time = 0
        _sinTime = 0
    }

    initialize() { }


    update(dt) {
        var ot = owner.getComponent(Transform)
        var ob = owner.getComponent(Body)
        var pt = Game.player.getComponent(Transform)

        var dir = pt.position - ot.position        
        dir.y = 0
        dir.normalise
        ob.velocity = dir * 1.5

        _sinTime = _sinTime + dt 
        ot.position.y = _sinTime.cos * 20 + 60
    }
}

import "game" for Game
import "unit" for Unit
