///////////////////////////////////////////////////////////////////////////////
// xs API
///////////////////////////////////////////////////////////////////////////////

class Color {
    construct new(r, g, b, a) {
        _r = r
        _g = g
        _b = b
        _a = a
    }
    construct new(r, g, b) {
        _r = r
        _g = g
        _b = b
        _a = 255
    }

    a { _a }
    r { _r }
    g { _g }
    b { _b }
    a=(v) { _a = v }
    r=(v) { _r = v }
    g=(v) { _g = v }
    b=(v) { _b = v }

    toNum { r << 24 | g << 16 | b << 8 | a }
    static fromNum(v) {
        var a = v & 0xFF
        var b = (v >> 8) & 0xFF
        var g = (v >> 16) & 0xFF
        var r = (v >> 24) & 0xFF
        return Color.new(r, g, b, a)
    }

    toString { "[r:%(_r) g:%(_g) b:%(_b) a:%(_a)]" }
}

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
    foreign static setColor_(color)
    static setColor(color) {
        var c = null
        if(color is Num) {
            c = color
        } else if( color is Color) {
            c = color.toNum
        }
        setColor_(c)
    }

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

    static circle(x, y, r, divs) {
        Render.begin(Render.lines)
        var t = 0.0
        var dt = (Num.pi * 2.0) / divs
        for(i in 0..divs) {            
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

    static arc(x, y, r, angle, divs) {
        
        var t = 0.0
        divs = angle / (Num.pi * 2.0) * divs
        divs = divs.truncate
        var dt = angle / divs
        if(divs > 0) {
            Render.begin(Render.lines)
            for(i in 0..divs) {
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

    static pie(x, y, r, angle, divs) {
        Render.begin(Render.triangles)
        var t = 0.0
        divs = angle / (Num.pi * 2.0) * divs
        divs = divs.truncate
        var dt = angle / divs
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

    foreign static loadImage(path)
    foreign static createSprite(imageId, x0, y0, x1, y1)
    foreign static renderSprite(spriteId, x, y, a)
    foreign static renderSprite(spriteId, x, y, size, rotation, mul, add, flags)
    foreign static setOffset(x, y)
    static renderSprite(spriteId, x, y) { renderSprite(spriteId, x, y, spriteBottom) }
    foreign static loadFont(font,size)
    foreign static renderText(fontId, text, x, y, mul, add, flags)

    static spriteBottom { 1 << 1 }
	static spriteCenter { 1 << 2 }
	static spriteFlipX  { 1 << 3 }
	static spriteFlipY  { 1 << 4 }
}

class File {
    foreign static read(src)
    foreign static write(text, dst)
}

class Input {
    foreign static getAxis(axis)
    foreign static getButton(button)
    foreign static getButtonOnce(button)
    foreign static getKey(key)
    foreign static getKeyOnce(key)

    // TODO: Add all keys
    static keyRight { 262 }
    static keyLeft  { 263 }
    static keyDown  { 264 }
    static keyUp    { 265 }
    static keySpace { 32  }
}

class Registry {
    static getNumber(name) { getNumber(name, game) }
    static getColorNum(name)  { getColorNum(name, game) }
    static getBool(name)  { getBool(name, game) }

    foreign static getNumber(name, type)
    foreign static getColorNum(name, type)
    foreign static getBool(name, type)

    foreign static setNumber(name, value, type)
    foreign static setColorNum(name, value, type)    
    foreign static setBool(name, value, type)

    static getColor(name) {
        var num = getColorNum(name)
        return Color.fromNum(num)
    }

    static setColor(name, value, type) {
        var c = null
        if(value is Num) {
            c = value
        } else if(value is Color) {
            c = value.toNum
        }
        setColorNum(name, c, type)
    }

    static system   { 2 }
	static debug    { 3 }
    static game     { 4 }
    static player   { 5 }
}
