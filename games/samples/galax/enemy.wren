import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Renderable, Body, Transform, Sprite

class Turret is Component {
    construct new() {
        super()
    }

    initialize() {
        _time = 20.0
    }

    /*
    update(dt) {
        var pe = Game.player
        var pt = pe.getComponent(Transform)
        var t = owner.getComponent(Transform)
        var d = pt.position - t.position
        var a = d.atan2
        a = a + Math.pi * 0.5
        t.rotation = a

        _time = _time + dt
        if(_time >= 1.0) {
            _time = 0.0
            Create.enemyProjectile(owner, d.normalise * 500.0, 5) 
        }
    }
    */

    static Cannon   { 1 }
    static Laser    { 2 }
    static Launcher { 3 }
    static Plasma   { 4 }
 }

 class Cannon is Turret {
    construct new() {
        super()
    }

    update(dt) {
        var pe = Game.player
        var pt = pe.getComponent(Transform)
        var t = owner.getComponent(Transform)
        var d = pt.position - t.position
        var a = d.atan2
        a = a + Math.pi * 0.5
        t.rotation = a

        _time = _time + dt
        if(_time >= 1.0) {
            _time = 0.0
            Create.enemyProjectile(owner, d.normalise * 500.0, 5) 
        }
    }
 }

 class Laser is Turret {
    construct new() { super() }
 }

 class Launcher is Turret {
    construct new() { super() }
 }

 class Plasma is Turret {
    construct new() { super() }
 }

 class Ship is Component {
    static lissajous(d, a, b, t ) {
        var x = (a * t + d).sin
        var y = (b * t).sin

        return Vec2.new(x, y)
    }

    construct new(t) {
        super()
        _d = Math.pi / 2.0
        _a = 3
        _b = 2
        _A = 200
        _B = 150
        _t = t
        _time = 0
    }

    initialize() {
        _transform = owner.getComponent(Transform)
        _body = owner.getComponent(Body)
    }

    update(dt) {
        _t = _t + dt * 0.9
        _t = _t % (2.0 * Math.pi)
        _time = _time + dt

        var pos = Ship.lissajous(_d, _a, _b, _t)
        pos.x = pos.x * _A
        pos.y = pos.y * _B

        //_transform.position = pos

        var pe = Game.player
        var pt = pe.getComponent(Transform)
        var t = owner.getComponent(Transform)
        var d = pt.position - t.position

        {
            var p = Game.player
            var d =  pos - _transform.position
            if(d.magnitude > 360) {
                d = d.normalise * 360
            }
            _body.velocity = d
            //var dv = d * Data.getNumber("Missle Max Speed")
            //dv = dv - b.velocity
        }

        if(_time >= 1.0) {
            _time = 0.0
            Create.enemyProjectile(owner, d.normalise * 50.0, 5) 
        }

        //_b.velocity = Vec2.new(_time.sin * 100.0, -10.0)
    }
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
            for(i in 0...5) {
                Create.Enemy(Vec2.new(200, 550), Size.S, 3, dvt * i)
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
