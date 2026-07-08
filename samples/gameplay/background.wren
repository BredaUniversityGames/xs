import "xs/core" for Render, Data
import "xs/math" for Math, Color

class Background {

    construct new() {
        _time = 0.0

        var image = Render.loadImage("[shared]/images/white.png")
        _sprite = Render.createSprite(image, 0, 0, 1, 1)
    }

    update(dt) { _time = _time + dt }

    render() {
        var fromColor = Data.getColor("From Color")
        var toColor = Data.getColor("To Color")
        fromColor = Color.fromNum(fromColor)
        toColor = Color.fromNum(toColor)
        for(i in 0...16) {            
            var x = (i + 1) * -128 + 640
            var offsetPos = (_time*0.5 + i ).sin^2 * 80.0
            var offsetRot = (_time*0.15 + i ).sin * 0.10
            var t = i / 16  
            var color = fromColor * (1 - t) + toColor * t
            Render.sprite(_sprite, x + offsetPos, Math.lerp(-360,-420, t), -i, 320, Math.pi * -0.15 + offsetRot, color.toNum, 0x00000000, 0)    
        }
    }

}