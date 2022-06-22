// import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
// import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
//import "unit" for Unit
//import "tags" for Team, Tag
//import "bullets" for Bullet
//import "debug" for DebugColor
import "random" for Random


class Boss is Component {
    static init() {
        __random = Random.new()
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
        ot.position.y = _sinTime.cos * 20 + 10

        _time = _time + dt
        if(_time > 1) {
            Create.bullet(owner, -1000, 100)
            _time = 0
        }
    }

    /*
    create() {

    } 
    */   

    generateDna(size) {
        var dna = ""
        for(i in 0...size) {
            var r = __random.int(0, 5)
            var l = __random.int(1, 2)
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
}

class Cannon is Component {
    construct new() { super() }
    
    shoot() {
    }
}

class Laser is Component {
    construct new() { super() }

    shoot() {
    }
}

class Missiles is Component {
    construct new() { super() }
    shoot() {

    }
}

import "game" for Game
import "create" for Create
