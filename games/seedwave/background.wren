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
        addLayer("[game]/assets/images/background/base.png", 35, 0)
        addLayer("[game]/assets/images/background/side.png", 55, 0)
    }

    static addLayer(relativeFilePath, speed, flags) {
        var img = Render.loadImage(relativeFilePath)
        var ch = Data.getNumber("Height", Data.system) * 0.5
        var cw = Data.getNumber("Width", Data.system) * 0.5
        var h = Render.getImageHeight(img)
        var y = -ch
        var repeat = (Data.getNumber("Height", Data.system) / h).ceil + 1

        for(i in 0..repeat) {
            var layer = Entity.new()
            var t = Transform.new(Vec2.new(-cw, y))
            var s = Sprite.new(relativeFilePath)
            var p = Parallax.new(speed, h, repeat)
            s.flags = flags
            layer.addComponent(t)
            layer.addComponent(p)
            layer.addComponent(s)
            layer.name = relativeFilePath
            y = y + h
        }       
    }

}

