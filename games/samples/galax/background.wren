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
        var bg = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))        
        var s = Sprite.new("[game]/assets/images/Background/stars_texture.png")
        s.layer = -1
        s.flags = Render.spriteCenter
        bg.addComponent(t)
        bg.addComponent(s)
        bg.name = "Background"

        /*
        addLayer("[game]/assets/images/Background/stars.png", 125, Math.radians(0))
        addLayer("[game]/assets/images/Background/stars.png", 100, Math.radians(90))
        addLayer("[game]/assets/images/Background/stars.png", 75, Math.radians(180))        
        addLayer("[game]/assets/images/Background/stars.png", 50, Math.radians(270))
        */
    }


    static addLayer(relativeFilePath, speed, rotation) {
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
            t.rotation = rotation
            layer.addComponent(t)
            layer.addComponent(p)
            layer.addComponent(s)
            layer.name = relativeFilePath
            y = y + h
        }       
    }

}

