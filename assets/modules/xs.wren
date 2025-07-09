///////////////////////////////////////////////////////////////////////////////
// xs API
///////////////////////////////////////////////////////////////////////////////

foreign class ShapeHandle {}    // Sprites are shapes as well

class Render {

    /// Sprite native API /////////////////////////////////////////////////////

    /// Load an image from a file and return an image id (supports png and jpg)
    foreign static loadImage(path)

    /// Load an shape from a file and return a shape id (supports svg)
    foreign static loadShape(path)

    /// Load a font from a file into a font atlas and return a font id
    foreign static loadFont(font, size)

    /// Get the width of an image
    foreign static getImageWidth(imageId)

    /// Get the height of an image
    foreign static getImageHeight(imageId)

    /// Create a sprite from section of an image
    foreign static createSprite(imageId, x0, y0, x1, y1)

    /// Create a mesh
    foreign static createShape(imageId, positions, textureCoords, indices)

    /// Destroy a shape
    foreign static destroyShape(shapeId)

    /// Set the offset for the next sprite(s) to be drawn
    foreign static setOffset(x, y)
    
    /// Draw a sprite at a position, sorted by z, with scale, rotation, colors and flags
    foreign static sprite(spriteId, x, y, z, scale, rotation, mul, add, flags)

    /// Draw a shape at a position, with scale, rotation, mul, and colors
    foreign static shape(shapeId, x, y, z, scale, rotation, mul, add)

    /// Draw text at a position, with scale, rotation, colors and flags
    foreign static text(fontId, txt, x, y, z, mul, add, flags)
    
    /// Don't apply any flags
    static spriteNone       { 0 << 0 }

    /// Draw the sprite at the bottom
    static spriteBottom     { 1 << 1 }

    /// Draw the sprite at the top
    static spriteTop        { 1 << 2 }

    /// Center the sprite on the x-axis
    static spriteCenterX    { 1 << 3 }

    /// Center the sprite on the y-axis
    static spriteCenterY    { 1 << 4 }

    /// Flip the sprite on the x-axis
    static spriteFlipX      { 1 << 5 }

    /// Flip the sprite on the y-axis
    static spriteFlipY      { 1 << 6 }

    /// Overlay the sprite as overlay (no offset applied)
    static spriteFixed    { 1 << 7 }

    /// Center the sprite on the x and y-axis
    static spriteCenter     { spriteCenterX | spriteCenterY }

    /// This is not a sprite but a shape, so handle it differently
    static spriteShape      { 1 << 8 }

    /// Debug native API //////////////////////////////////////////////////////

    /// Primitive type lines    
    static lines { 0 }

    /// Primitive type triangles
    static triangles { 1 }

    /// Begin a debug draw
    foreign static dbgBegin(primitive)

    /// End a debug draw (flush)
    foreign static dbgEnd()

    /// Add a vertex
    foreign static dbgVertex(x, y)

    /// Set the color for the next vertex
    foreign static dbgColor(color)

    /// Draw a line
    foreign static dbgLine(x0, y0, x1, y1)

    /// Draw some text using dbgSquare font (debug)
    foreign static dbgText(text, x, y, size)

    /// Helper functions //////////////////////////////////////////////////////

    static dbgLine(a, b) {
        dbgLine(a.x, a.y, b.x, b.y)
    }

    static dbgRect(fromX, fromY, toX, toY) {
        Render.dbgBegin(Render.triangles)
            Render.dbgVertex(fromX, fromY)
            Render.dbgVertex(toX, fromY)
            Render.dbgVertex(toX, toY)

            Render.dbgVertex(fromX, fromY)
            Render.dbgVertex(fromX, toY)
            Render.dbgVertex(toX, toY)
        Render.dbgEnd()
    }

    static dbgSquare(centerX, centerY, size) {
        var s = size * 0.5
        Render.dbgRect(centerX - s, centerY - s, centerX + s, centerY + s)
    }

    static dbgDisk(x, y, r, divs) {
        Render.dbgBegin(Render.triangles)
        var t = 0.0
        var dt = (Num.pi * 2.0) / divs
        for(i in 0...divs) {            
            Render.dbgVertex(x, y)
            var xr = t.cos * r            
            var yr = t.sin * r
            Render.dbgVertex(x + xr, y + yr)
            t = t + dt
            xr = t.cos * r
            yr = t.sin * r
            Render.dbgVertex(x + xr, y + yr)
        }
        Render.dbgEnd()
    }

    static dbgCircle(x, y, r, divs) {
        Render.dbgBegin(Render.lines)
        var t = 0.0
        var dt = (Num.pi * 2.0) / divs
        for(i in 0..divs) {            
            var xr = t.cos * r            
            var yr = t.sin * r
            Render.dbgVertex(x + xr, y + yr)
            t = t + dt
            xr = t.cos * r
            yr = t.sin * r
            Render.dbgVertex(x + xr, y + yr)
        }
        Render.dbgEnd()
    }

    static dbgArc(x, y, r, angle, divs) {        
        var t = 0.0
        divs = angle / (Num.pi * 2.0) * divs
        divs = divs.truncate
        var dt = angle / divs
        if(divs > 0) {
            Render.dbgBegin(Render.lines)
            for(i in 0..divs) {
                var xr = t.cos * r            
                var yr = t.sin * r
                Render.dbgVertex(x + xr, y + yr)
                t = t + dt
                xr = t.cos * r
                yr = t.sin * r
                Render.dbgVertex(x + xr, y + yr)
            }
            Render.dbgEnd()
        }
    }

    static dbgPie(x, y, r, angle, divs) {
        Render.dbgBegin(Render.triangles)
        var t = 0.0
        divs = angle / (Num.pi * 2.0) * divs
        divs = divs.truncate
        var dt = angle / divs
        for(i in 0..divs) {            
            Render.dbgVertex(x, y)
            var xr = t.cos * r            
            var yr = t.sin * r
            Render.dbgVertex(x + xr, y + yr)
            t = t + dt
            xr = t.cos * r
            yr = t.sin * r
            Render.dbgVertex(x + xr, y + yr)
        }
        Render.dbgEnd()
    }

    static dbgVertex(v) {
        Render.dbgVertex(v.x, v.y)
    }

    static sprite(spriteId, x, y) {
        sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }

    static sprite(spriteId, x, y, z) {
        sprite(spriteId, x, y, z, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }

    static sprite(spriteId, x, y, z, flags) {
        sprite(spriteId, x, y, z, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, flags)
    }

    static createGridSprite(imageId, columns, rows,  c, r) {
        var ds = 1 / columns
        var dt = 1 / rows        
        var s = c * ds
        var t = r * dt
        return createSprite(imageId, s, t, s + ds, t + dt)
    }

    static createGridSprite(imageId, columns, rows,  idx) { 
        var ds = 1 / columns
        var dt = 1 / rows
        var r = (idx / columns).truncate
        var c = idx % columns
        var s = c * ds
        var t = r * dt
        System.print("imageId: %(imageId), columns:%(columns), rows:%(rows),  idx:%(idx)")
        System.print("c: %(c), r:%(r), s:%(s),  t:%(t)")
        return createSprite(imageId, s, t, s + ds, t + dt)
    }
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
    foreign static getAxisOnce(axis, threshold)
    foreign static getButton(button)
    foreign static getButtonOnce(button)

    foreign static getKey(key)
    foreign static getKeyOnce(key)

    foreign static getMouse()
    foreign static getMouseButton(button)
    foreign static getMouseButtonOnce(button)
    foreign static getMouseX()
    foreign static getMouseY()
    foreign static getMouseWheel()

    foreign static getNrTouches()
    foreign static getTouchId(index)
    foreign static getTouchX(index)
    foreign static getTouchY(index)

    foreign static setPadVibration(lowRumble, highRumble, time)
    foreign static setPadLightbarColor(red, green, blue)
    foreign static resetPadLightbarColor()

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
    
    static gamepadButtonSouth      { 0  }
    static gamepadButtonEast       { 1  }
    static gamepadButtonWest       { 2  }
    static gamepadButtonNorth      { 3  }
    static gamepadShoulderLeft     { 4  }
    static gamepadShoulderRight    { 5  }
    static gamepadButtonSelect     { 6  }
    static gamepadButtonStart      { 6  }
    
    static gamepadLeftStickPress   { 9  }
    static gamepadRightStickPress  { 10 }
    static gamepadDPadUp           { 11 }
    static gamepadDPadDown         { 12 }
    static gamepadDPadLeft         { 13 }
    static gamepadDPadRight        { 14 }        

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

class Audio {

    foreign static load(name, groupId)
    foreign static play(soundId)
    foreign static getGroupVolume(groupId)
    foreign static setGroupVolume(groupId, volume)
    foreign static getChannelVolume(channelId)
    foreign static setChannelVolume(channelId, volume)
    foreign static getBusVolume(busName)
    foreign static setBusVolume(busName, volume)
    foreign static loadBank(bankId)
    foreign static unloadBank(bankId)
    foreign static startEvent(eventName)
    foreign static setParameterNumber(eventId, paramName, newValue)
    foreign static setParameterLabel(eventId, paramName, newValue)

    static groupSFX    { 1 }
    static groupMusic  { 2 }
}

class Data {
    static getNumber(name) { getNumber(name, game) }
    static getColor(name)  { getColor(name, game) }
    static getBool(name)  { getBool(name, game) }

    foreign static getNumber(name, type)
    foreign static getColor(name, type)
    foreign static getBool(name, type)
    foreign static getString(name, type)

    foreign static setNumber(name, value, type)
    foreign static setColor(name, value, type)    
    foreign static setBool(name, value, type)
    foreign static setString(name, value, type)

    static system   { 2 }
	static debug    { 3 }
    static game     { 4 }
    static player   { 5 }
}

class Device {

    foreign static getPlatform()
    foreign static canClose()
    foreign static requestClose()

    static PlatformPC      { 0 }
    static PlatformPS5     { 1 }
    static PlatformSwitch  { 2 }
}

class Profiler {
    foreign static begin(name)
    foreign static end(name)
}
