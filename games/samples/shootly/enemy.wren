import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Renderable, Body, Transform, Sprite

class Turret is Component {
    construct new() {
        super()
    }

    initialize() {
        _sprite = owner.getComponentSuper(Sprite)
    }

    update(dt) {
        var pe = Game.player
        var pt = pe.getComponent(Transform)
        var t = owner.getComponent(Transform)
        var d = pt.position - t.position
        var a = d.atan2
        a = a + Math.pi * 0.5
        _sprite.rotation = a
    }

    static Cannon   { 1 }
    static Laser    { 2 }
    static Missile  { 3 }
    static Plasma   { 4 }
 }

import "game" for Game 
import "debug" for DebugColor
import "tags" for Tag, Team, Size
import "unit" for Unit
import "random" for Random
import "create" for Create 
