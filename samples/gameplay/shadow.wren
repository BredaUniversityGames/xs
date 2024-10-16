import "xs" for Input, Render, Data, File
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation, Label
import "random" for Random


class Shadow is  Component {    
    construct new() {
        _offset = Vec2.new(10, -10)
    }

    initialize() {
        _transform = owner.getComponent(Transform)
    }

    offset { _offset }

    static render() {    
        for(e in Entity.entities) {
            var s = e.getComponentSuper(Sprite)
            if(s != null) {
                // s.render()
                var sh = e.getComponent(Shadow)                
                var t = e.getComponent(Transform)
                var color = Data.getColor("Shadow Color")
                if(sh != null && t != null) {
                    Render.sprite(
                    s.sprite,
                    t.position.x + sh.offset.x,
                    t.position.y + sh.offset.y,
                    s.layer - 1.0,
                    s.scale,
                    t.rotation,
                    color,
                    s.add,
                    s.flags)
                }
            }
        }
    }
}
