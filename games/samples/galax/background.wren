import "xs" for Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Vec2, Math
import "xs_components" for Transform, Sprite

// Helper code (no need to touch)
class Parallax is Component {
    construct new(speed, height, repeat) {
        _speed = speed
        _height = height
        _repeat = repeat
    }

    update(dt) {        
        var t = owner.getComponent(Transform)
        t.position.y = t.position.y - dt * _speed

        if(t.position.y < Data.getNumber("Height", Data.system) * -0.5 -_height) {
            t.position.y = t.position.y + _height * _repeat
        }
    }
}

class Background {
    static createBackground() {        
        {
            var bg = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = Sprite.new("[game]/assets/images/Background/blue.png")
            s.layer = -100
            s.flags = Render.spriteCenter
            bg.addComponent(t)
            bg.addComponent(s)
            bg.name = "Background"
            s.scale = 70
        }

        addLayer("[game]/assets/images/Background/stars.png", 35, 0)
        addLayer("[game]/assets/images/Background/stars.png", 55, Render.spriteFlipX)
        addLayer("[game]/assets/images/Background/stars.png", 75, Render.spriteFlipY)

        {
            var fg = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = Sprite.new("[game]/assets/images/Background/frame.png")
            s.layer = 100
            s.flags = Render.spriteCenter
            fg.addComponent(t)
            fg.addComponent(s)
            fg.name = "Foreground"
        }
    }


    static addLayer(relativeFilePath, speed, flags) {
        var img = Render.loadImage(relativeFilePath)
        var ch = Data.getNumber("Height", Data.system) * 0.5
        var cw = Data.getNumber("Width", Data.system) * 0.5
        //var w = Render.getImageWidth(img)
        var h = Render.getImageHeight(img)
        var y = -ch
        var repeat = (Data.getNumber("Height", Data.system) / h).ceil + 1

        for(i in 0..repeat) {
            var layer = Entity.new()
            var t = Transform.new(Vec2.new(-cw, y))
            var s = Sprite.new(relativeFilePath)
            var p = Parallax.new(speed, h, repeat)
            s.flags = flags
            //t.rotation = rotation
            layer.addComponent(t)
            layer.addComponent(p)
            layer.addComponent(s)
            layer.name = relativeFilePath
            y = y + h
        }       
    }

}

