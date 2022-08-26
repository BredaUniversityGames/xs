///////////////////////////////////////////////////////////////////////////////
// xs API
///////////////////////////////////////////////////////////////////////////////

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
    foreign static setColor(r, g, b, a)
    foreign static setColor(color)
    foreign static line(x0, y0, x1, y1)
    foreign static text(text, x, y, size)

    static triangles { 1 }
    static lines { 2 }
    foreign static begin(primitive)
    foreign static end()
    foreign static vertex(x, y)

    static setColor(r, g, b) { Render.setColor(r, g, b, 1.0) }

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
    foreign static getImageWidth(imageId)
    foreign static getImageHeight(imageId)
    foreign static createSprite(imageId, x0, y0, x1, y1)
    foreign static renderSprite(spriteId, x, y, scale, rotation, mul, add, flags)
    foreign static setOffset(x, y)    
    foreign static loadFont(font,size)
    foreign static renderText(fontId, text, x, y, mul, add, flags)

    static renderSprite(spriteId, x, y) {
        renderSprite(spriteId, x, y, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }
    static spriteBottom { 1 << 1 }
	static spriteCenter { 1 << 2 }
	static spriteFlipX  { 1 << 3 }
	static spriteFlipY  { 1 << 4 }
}

class File {
    foreign static read(src)
    foreign static write(text, dst)
    foreign static exists(src)
}

class TouchData {
    construct new(index, x, y) {
        _index = index
        _x = x
        _y = y
    }

    index { _index }
    x { _x }
    y { _y }
}

class Input {
    foreign static getAxis(axis)
    foreign static getButton(button)
    foreign static getButtonOnce(button)
    foreign static getKey(key)
    foreign static getKeyOnce(key)
    foreign static getMouseButton(button)
    foreign static getMouseButtonOnce(button)
    foreign static getMouseX()
    foreign static getMouseY()

    foreign static getNrTouches()
    foreign static getTouchId(index)
    foreign static getTouchX(index)
    foreign static getTouchY(index)

    static getTouchData() {
        var nrTouches = getNrTouches()
        var result = []
        for (i in 0...nrTouches) result.add(getTouchData(i))
        return result
    }

    static getTouchData(index) {
        return TouchData.new(getTouchId(index), getTouchX(index), getTouchY(index))
    }

    static getMousePosition() {
        return [getMouseX(), getMouseY()]
    }

    static keyRight  { 262 }
    static keyLeft   { 263 }
    static keyDown   { 264 }
    static keyUp     { 265 }
    static keySpace  { 32  }
    static keyEscape { 256 }
    static keyEnter  { 257 }

    static keyA { 65 }
    static keyB { 66 }
    static keyC { 67 }
    static keyD { 68 }
    static keyE { 69 }
    static keyF { 70 }
    static keyG { 71 }
    static keyH { 72 }
    static keyI { 73 }
    static keyJ { 74 }
    static keyK { 75 }
    static keyL { 76 }
    static keyM { 77 }
    static keyN { 78 }
    static keyO { 79 }
    static keyP { 80 }
    static keyQ { 81 }
    static keyR { 82 }
    static keyS { 83 }
    static keyT { 84 }
    static keyU { 85 }
    static keyV { 86 }
    static keyW { 87 }
    static keyX { 88 }
    static keyY { 89 }
    static keyZ { 90 }

    // TODO: add more keys

    static gamepadButtonSouth      { 0  }
    static gamepadButtonEast       { 1  }
    static gamepadButtonWest       { 2  }
    static gamepadButtonNorth      { 3  }
    static gamepadShoulderLeft     { 4  }
    static gamepadShoulderRight    { 5  }
    static gamepadButtonShare      { 6  }
    static gamepadButtonMenu       { 7  }
    
    static gamepadLeftStickPress   { 9  }
    static gamepadRightStickPress  { 10 }
    static gamepadDPadUp           { 11 }
    static gamepadDPadRight        { 12 }
    static gamepadDPadDown         { 13 }
    static gamepadDPadLeft         { 14 }

    static gamepadAxisLeftStickX   { 0  }
    static gamepadAxisLeftStickY   { 1  }
    static gamepadAxisRightStickX  { 2  }
    static gamepadAxisRightStickY  { 3  }
    static gamepadAxisLeftTrigger  { 4  }
    static gamepadAxisRightTrigger { 5  }

    static mouseButtonLeft   { 0 }
    static mouseButtonRight  { 1 }
    static mouseButtonMiddle { 2 }
}

class Registry {
    static getNumber(name) { getNumber(name, game) }
    static getColor(name)  { getColor(name, game) }
    static getBool(name)  { getBool(name, game) }

    foreign static getNumber(name, type)
    foreign static getColor(name, type)
    foreign static getBool(name, type)

    foreign static setNumber(name, value, type)
    foreign static setColor(name, value, type)    
    foreign static setBool(name, value, type)

    static system   { 2 }
	static debug    { 3 }
    static game     { 4 }
    static player   { 5 }
}
