import "xs" for Render
import "xs_ec"for Entity, Component
import "xs_math"for Vec2
import "xs_components" for Transform, Sprite

class Background {
    static createBackground() {
        var bg = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))        
        var s = Sprite.new("[game]/assets/images/Background/stars_texture.png")
        s.layer = -1
        s.flags = Render.spriteCenter
        bg.addComponent(t)
        bg.addComponent(s)
        bg.name = "Background"
    }
}

