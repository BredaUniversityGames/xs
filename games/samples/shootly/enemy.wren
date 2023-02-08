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
            Create.enemyProjectile(owner, d.normal * 500.0, 5) 
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
            Create.enemyProjectile(owner, d.normal * 500.0, 5) 
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
    construct new() {
        super()
    }

    initialize() {
        var b = owner.getComponent(Body)
        b.velocity = Vec2.new(0.0, -56.0)
    }

    update(dt) {

    }
 }

 class Waves {
    static init() {
        __time = 20.0
        __wave = 1
    }

    static update(dt) {
        __time = __time + dt
        if(__time > 15.0) {
            for(i in 0...6) {
                Create.Enemy(Vec2.new(-200 + i * 100, 250), Size.S, i % 3)
            }
            __wave = __wave + 1
            __time = 0
        }
    }
 }

import "game" for Game 
import "debug" for DebugColor
import "tags" for Tag, Team, Size
import "unit" for Unit
import "random" for Random
import "create" for Create
