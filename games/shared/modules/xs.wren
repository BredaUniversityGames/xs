
class Configuration {
    foreign static title=(value) 
    foreign static title
    foreign static width=(value)
    foreign static width
    foreign static height=(value) 
    foreign static height
    foreign static multiplier=(value) 
    foreign static multiplier
}

class Render {
    foreign static setColor(r, g, b)
    foreign static setColor(hexString)
    foreign static polygon(x, y, radius, sides)
    foreign static rect(x, y, sizeX, sizeY, rotation)
    foreign static line(x0, y0, x1, y1)
    foreign static text(text, x, y, size)

    static triangles { 1 }
    static lines { 2 }
    foreign static begin(primitive)
    foreign static end()
    foreign static vertex(x, y)

    static rect(fromX, fromY, toX, toY) {
        Render.begin(Render.triangles)
            Render.vertex(fromX, fromY)
            Render.vertex(toX, fromY)
            Render.vertex(toX, toY)

            Render.vertex(fromX, fromY)
            Render.vertex(fromX, toY)
            Render.vertex(toX, toY)
        Render.end()
    }

    static square(centerX, centerY, size) {
        var s = size * 0.5
        Render.rect(centerX - s, centerY - s, centerX + s, centerY + s)
    }

    static disk(x, y, r, divs) {
        Render.begin(Render.triangles)
        var t = 0.0
        var dt = (Num.pi * 2.0) / divs
        for(i in 0..divs) {            
            Render.vertex(x, y)
            var xr = t.cos * r            
            var yr = t.sin * r
            Render.vertex(x + xr, y + yr)
            t = t + dt
            xr = t.cos * r
            yr = t.sin * r
            Render.vertex(x + xr, y + yr)
        }
        Render.end()
    }
}

class Math {
    static lerp(a, b, t) {
        return (a * (1.0 - t)) + (b * t) 
    }

    static damp(a, b, lambda, dt) {
        return lerp(a, b, 1.0 - (-lambda * dt).exp)
    }    
}

class Input {
    foreign static getAxis(axis)
    foreign static getButton(button)
    foreign static getButtonOnce(button)
    foreign static getKey(key)
    foreign static getKeyOnce(key)

    static keyRight { 262 }
    static keyLeft  { 263 }
    static keyDown  { 264 }
    static keyUp    { 265 }
    static keySpace { 32  }
}

