import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor
import "components" for SlowRelation
import "random" for Random

/*
class Point
{
    construct new(x,y) {
        _x = x
        _y = y
    }

    x { _x }
    y { _y }
}
*/

class Create {

    static init() {
        __random = Random.new()

        __offsets = []

        /*
        __offsets.add(getOffset(0, 0))
        __offsets.add(getOffset(0, 1))
        __offsets.add(getOffset(0, -1))
        __offsets.add(getOffset(1, 0))
        __offsets.add(getOffset(1, 1))
        __offsets.add(getOffset(1, -1))
        __offsets.add(getOffset(2, 0))
        __offsets.add(getOffset(2, 1))
        __offsets.add(getOffset(2, -1))
        __offsets.add(getOffset(3, 0))
        __offsets.add(getOffset(3, 1))
        __offsets.add(getOffset(3, -1))
        __offsets.add(getOffset(4, 0))
        __offsets.add(getOffset(4, 1))
        __offsets.add(getOffset(4, -1))
        __offsets.add(getOffset(5, 0))
        __offsets.add(getOffset(5, 1))
        __offsets.add(getOffset(5, -1))
        */

        __offsets.add(Vec2.new(0, 0))
        __offsets.add(Vec2.new(0, 1))
        __offsets.add(Vec2.new(0, -1))
        __offsets.add(Vec2.new(1, 0))
        __offsets.add(Vec2.new(1, 1))
        __offsets.add(Vec2.new(1, -1))
        __offsets.add(Vec2.new(2, 0))
        __offsets.add(Vec2.new(2, 1))
        __offsets.add(Vec2.new(2, -1))
        __offsets.add(Vec2.new(3, 0))
        __offsets.add(Vec2.new(3, 1))
        __offsets.add(Vec2.new(3, -1))
        __offsets.add(Vec2.new(4, 0))
        __offsets.add(Vec2.new(4, 1))
        __offsets.add(Vec2.new(4, -1))
        __offsets.add(Vec2.new(5, 0))
        __offsets.add(Vec2.new(5, 1))
        __offsets.add(Vec2.new(5, -1))

    }

    static getOffset(orbit, slot, maxOrbit) {
        var dr = Registry.getNumber("Boss Orbit Radius")
        var rad = orbit * dr + Registry.getNumber("Boss Core 1 Radius")        
        if(maxOrbit > rad) {
            rad = maxOrbit
        }
        var dl = Registry.getNumber("Boss Orbit Angle") / rad
        var a = dl * slot        
        var pos = Vec2.new(a.cos * rad, a.sin * rad)
        //System.print(pos)
        return pos
    }

    static player() {
        var ship = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()
        var v = Vec2.new(0, 0)
        var b = Body.new(Registry.getNumber("Player Size"), v)
        var u = Unit.new(Team.player, Registry.getNumber("Player Health"))
        var c = DebugColor.new(0x8BEC46FF)
        var s = GridSprite.new("[games]/seedwave/assets/images/ships/planes_05_A.png", 4, 5)
        s.layer = 1.0
        s.flags = Render.spriteCenter
        ship.addComponent(t)
        ship.addComponent(sc)            
        ship.addComponent(b)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(s)
        ship.name = "Player"
        ship.tag = (Tag.Player | Tag.Unit)
        return ship
    }

    static playerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0, 35))
        var v = Vec2.new(0, speed)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.player, damage)
        var s = AnimatedSprite.new("[games]/seedwave/assets/images/projectiles/projectile-06-02.png", 3, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("anim", [0,1,2])
        s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }

    static bullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 35))
        var v = Vec2.new(0, speed)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.player, damage)
        var s = AnimatedSprite.new("[games]/seedwave/assets/images/projectiles/projectile-06-02.png", 3, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("anim", [0,1,2])
        s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }

    static part(boss, type, level, position) {
        var l = __random.float(
            Registry.getNumber("Part delay lambda from"),
            Registry.getNumber("Part delay lambda to"))

        var part = Entity.new()
        var rad = Registry.getNumber("Part Size") + level * 5
        var t = Transform.new(Vec2.new(0, 0))
        var b = Body.new(rad, Vec2.new(0, 0))
        var u = Unit.new(Team.player, Registry.getNumber("Player Health"))
        var c = DebugColor.new(0x8BEC46FF)        
        var r = SlowRelation.new(boss, l)
        r.offset = position
        part.addComponent(t)
        //part.addComponent(bs)
        part.addComponent(b)
        part.addComponent(u)
        part.addComponent(c)        
        part.addComponent(r)
        part.name = "Part"
        part.tag = (Tag.Computer | Tag.Unit)

        var s = GridSprite.new("[games]/seedwave/assets/images/ships/turret_medium_64x64.png", 3, 4)
        if(type == "C") {
            s.idx = 0 + level - 1
        } else if (type == "L") {
            s.idx = 4 + level - 1
        } else if(type == "M") {
            s.idx = 8 + level - 1
        }         
        s.layer = 1.0
        s.flags = Render.spriteCenter | Render.spriteFlipY // |
        part.addComponent(s)
    }

    static generateDna(size) {
        var dna = ""
        for(i in 0...size) {
            var r = __random.int(0, 4)
            var l = __random.int(0, 3)
            if(r == 0) {
                dna = dna + "S"
            } else if(r == 1) {
                dna = dna + "C" + l.toString
            } else if(r == 2) {
                dna = dna + "L" + l.toString
            } else if(r == 3) {
                dna = dna + "M" + l.toString
            }
        }
        return dna
    }

    static randomBoss(size) {
        var dna = generateDna(size)
        System.print(dna)
        return boss(dna)
    }

    static boss(dna) {
        var ship = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var bs = Boss.new()
        var v = Vec2.new(0, 0)
        var b = Body.new(Registry.getNumber("Core Size"), v)
        var u = Unit.new(Team.player, Registry.getNumber("Player Health"))
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
        ship.tag = (Tag.Computer | Tag.Unit)

        var type = null
        var flip = false
        var idx = 0
        var offsets = [Vec2.new(0,0)]
        var radii = [Registry.getNumber("Core Size")]        
        var maxOrbit = 0        
        for(i in dna) {
            var level = Num.fromString(i)
            if(level != null && type != null) {
                var pos = Vec2.new(__offsets[idx].x, __offsets[idx].y)                
                pos = getOffset(pos.x, pos.y, maxOrbit)
                var rad = Registry.getNumber("Part Size") + level * 5

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
}

import "player" for Player
import "boss" for Boss
