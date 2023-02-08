import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Renderable, Body, Transform, Sprite

class Formation is Component {
    construct new(a, b, A, B) {
        super()
        _d = Math.pi / 2.0
        _a = a
        _b = b
        _A = A
        _B = B
        _time = 0
        _ships= []
    }   

    initialize() {
        _transform = owner.getComponent(Transform)
    } 

    calculatePoint(t) {
        var x = (_a * t + _d).sin * _A
        var y = (_b * t).sin * _B
        return Vec2.new(x, y) + _transform.position
    }

    update(dt) {}

    add(ship) {
        _ships.add(ship)
    }
}

class Ship is Component {   
    construct new(formation, t) {
        super()
        _formation = formation
        _t = t
        _time = 0
    }

    initialize() {
        _transform = owner.getComponent(Transform)
        _body = owner.getComponent(Body)
        _formation.add(owner)
    }

    update(dt) {
        _t = _t + dt * 0.9
        _t = _t % (2.0 * Math.pi)
        _time = _time + dt

        var pos = _formation.calculatePoint(_t)

        var pe = Game.player
        var pt = pe.getComponent(Transform)
        var t = owner.getComponent(Transform)
        var d = pt.position - t.position

        {
            var p = Game.player
            var d =  pos - _transform.position
            if(d.magnitude > 360) {
                d = d.normal * 360
            }
            _body.velocity = d
        }

        if(_time >= 1.0) {
            _time = 0.0
            Create.enemyPlasma(owner, d.normal * 50.0, 1) 
        }
    }

    static plasma   { 1 }
    static fire     { 2 }
    static cannon   { 3 }
 }

 class Waves {
    static init() {
        __time = 20.0
        __wave = 1
    }

    static update(dt) {
        __time = __time + dt

        if(__time > 1.0) {            
            var dvt = 0.3            
            var e = Entity.new()
            var t  = Transform.new(Vec2.new(0, 0))
            var f = Formation.new(3, 2, 200, 150)
            e.addComponent(t)
            e.addComponent(f)
            var type = Game.random.int(1, 3)
            var diff = Game.random.int(1, 4)

            for(i in 0...5) {
                Create.enemy(Vec2.new(200, 550), type, diff, f, dvt * i)
                __wave = __wave + 1
                __time = 0
            }
        }
    }
 }

import "game" for Game 
import "debug" for DebugColor
import "tags" for Tag, Team, Size
import "unit" for Unit
import "random" for Random
import "create" for Create
