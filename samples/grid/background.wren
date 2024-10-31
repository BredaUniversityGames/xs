import "xs" for Render, Data
import "xs_math" for Math, Color

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
            var x = (i + 1) * -64 + 320
            var offset = (_time + i ).sin * 10.0
            var t = i / 16  
            var color = fromColor * (1 - t) + toColor * t
            Render.sprite(_sprite, x + offset, -180, -i - 100, 200, Math.pi * -0.25, color.toNum, 0x00000000, 0)    
        }
    }
}