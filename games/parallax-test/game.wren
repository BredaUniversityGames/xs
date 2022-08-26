import "xs" for Configuration, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Vec2
import "xs_components" for Transform, Renderable, Sprite

// Helper code (no need to touch)
class Parallax is Component {
    construct new(speed, width, repeat) {
        _speed = speed
        _width = width
        _repeat = repeat
    }

    update(dt) {        
        var t = owner.getComponent(Transform)
        t.position.x = t.position.x - dt * _speed

        if(t.position.x < Configuration.width * -0.5 -_width) {
            t.position.x = t.position.x + _width * _repeat
        }
    }
}

class Game {
    static config() {
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 2
        Configuration.title = "Parallax Test for Rumena"
    }

    static init() {        
        Entity.init()

        // TODO: Rumena, add/remove layers here
        // Every line that starts with the slashes is a comment. Humans can see it,
        // the computer ignores it. You can use it to turn lines on and off
        // 
        // First argument is the name of the layer image
        // Second is how many time you want to repeat it
        // Third is scroll speed

        // Blue sky
        // addLayer("[games]/parallax-test/images/blue-sky/sky.png", 5)
        // addLayer("[games]/parallax-test/images/blue-sky/far-buildings.png", 35)
        // addLayer("[games]/parallax-test/images/blue-sky/buildings.png", 75)
        // addLayer("[games]/parallax-test/images/blue-sky/trees.png", 100)

        // Pink
        // addLayer("[games]/parallax-test/images/pink/back.png", 5)
        // addLayer("[games]/parallax-test/images/pink/mountains.png", 35)
        // addLayer("[games]/parallax-test/images/pink/buildings.png", 75)

        // Glacial
        addLayer("games/parallax-test/images/glacial/sky.png", 5)
        addLayer("games/parallax-test/images/glacial/clouds_bg.png", 10)
        addLayer("games/parallax-test/images/glacial/cloud_lonely.png", 20)
        addLayer("games/parallax-test/images/glacial/glacial_mountains.png", 40)
        addLayer("games/parallax-test/images/glacial/clouds_mg_3.png", 60)
        addLayer("games/parallax-test/images/glacial/clouds_mg_2.png", 70)
        addLayer("games/parallax-test/images/glacial/clouds_mg_1.png", 80)
    }    
    
    static update(dt) {
        Entity.update(dt)
    }

    static render() {
        Renderable.render()
    }

    static addLayer(relativeFilePath, speed) {
        var img = Render.loadImage(relativeFilePath)
        var ch = Configuration.height * 0.5
        var cw = Configuration.width * 0.5
        var w = Render.getImageWidth(img)
        var x = -cw
        var repeat = (Configuration.width / w).ceil + 1

        for(i in 0..repeat) {
            var layer = Entity.new()
            var t = Transform.new(Vec2.new(x, -ch))    
            var s = Sprite.new(relativeFilePath)
            var p = Parallax.new(speed, w, repeat)
            layer.addComponent(t)
            layer.addComponent(p)
            layer.addComponent(s)
            layer.name = relativeFilePath
            x = x + w
        }       
    }
}
