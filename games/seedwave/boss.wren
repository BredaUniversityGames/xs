 import "xs" for Configuration, Input, Render, Data
import "xs_ec"for Entity, Component
 import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet, Missile
import "debug" for DebugColor
import "random" for Random
import "components" for SlowRelation

class Boss is Component {
        static init() {
        __random = Random.new()
        __offsets = []

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
            var r = __random.int(-1, 5)
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
        var u = Unit.new(Team.Computer, health)
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

        var s = GridSprite.new("[games]/seedwave/assets/images/ships/turret_medium_64x64.png", 3, 4)
        if(type == "C") {                                 //  Cannon
            s.idx = 0 + level - 1
            c = DebugColor.new(0xFFA8D3FF)
            var w = Cannon.new()
            part.addComponent(w)
        } else if (type == "L") {                           // Laser
            s.idx = 4 + level - 1
            c = DebugColor.new(0x9896FFFF)
        } else if(type == "M") {                            // Missiles
            s.idx = 8 + level - 1
            c = DebugColor.new(0xFDFFC1FF)
            var w = Missiles.new()
            part.addComponent(w)
        } else if(type == "D") {                            // Deflect
            part.tag = (part.tag| Tag.Deflect)
            s.idx = 8 + level - 1
            c = DebugColor.new(0xFFB599FF)
        }
        s.layer = 1.0
        s.flags = Render.spriteCenter | Render.spriteFlipY // |
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
        var u = Unit.new(Team.Computer, Data.getNumber("Core Health"))
        var c = DebugColor.new(0x8BEC46FF)
        var s = GridSprite.new("[games]/seedwave/assets/images/ships/ship_medium_64x128.png", 3, 1)
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
            dna = generateDna(10)
        }
        return create(dna)
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

class Cannon is Component {
    construct new() {
        super()
        _time = 0        
    }

    update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Cannon Shoot Time")) {
            Bullet.create(owner, -Data.getNumber("Cannon Round Speed"), 0)
            _time = 0
        }
    }
    
    shoot() {
    }
}

class Laser is Component {
    construct new() { super() }

    shoot() {
    }
}

class Missiles is Component {
    construct new() {
        super()
        _time = 0
    }

     update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Missle Shoot Time")) {            
            Missile.create(owner,
                Data.getNumber("Missle Speed"),
                Data.getNumber("Missle Damage"))
            _time = 0
        }
    }

    shoot() {
    }

    toString { "[Missiles _time:%(_time)] ->" + super.toString() }
}

import "game" for Game
import "create" for Create
