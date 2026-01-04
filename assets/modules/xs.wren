/// Handle for shapes and sprites in the rendering system.
/// Used to correctly manage resources via the GC. Not to be created directly.
foreign class ShapeHandle {}

/// Core rendering API for sprites, shapes, text, and debug drawing
/// Provides functionality for rendering images, texts and shapes
class Render {

    /// Loads an image from a file and returns an image ID
    /// Supports PNG and JPG formats. Use relative paths like "[game]/textures/flower.png"
    foreign static loadImage(path)

    /// Loads a shape from a file and returns a shape ID
    /// Supports SVG format
    foreign static loadShape(path)

    /// Loads a font into a font atlas and returns a font ID
    /// Font will be rasterized at the specified size
    foreign static loadFont(font, size)

    /// Gets the width in pixels of a loaded image
    foreign static getImageWidth(imageId)

    /// Gets the height in pixels of a loaded image
    foreign static getImageHeight(imageId)

    /// Creates a sprite from a section of an image using texture coordinates
    /// Coordinates are normalized (0.0 to 1.0): x0, y0 (top-left), x1, y1 (bottom-right)
    foreign static createSprite(imageId, x0, y0, x1, y1)

    /// Creates a custom mesh shape from vertices, texture coordinates, and indices
    foreign static createShape(imageId, positions, textureCoords, indices)

    /// Destroys a shape and frees its resources
    foreign static destroyShape(shapeId)

    /// Sets the offset for subsequent sprite draw calls
    /// All sprites will be offset by (x, y) until new values are set
    foreign static setOffset(x, y)

    /// Draws a sprite with full control over appearance
    /// - spriteId: Valid sprite ID created with createSprite (not an image ID)
    /// - x, y: Position on screen (affected by setOffset)
    /// - z: Sorting depth value
    /// - scale: Scaling factor
    /// - rotation: Rotation angle in radians
    /// - mul: Multiply color (0xRRGGBBAA format)
    /// - add: Additive color (0xRRGGBBAA format)
    /// - flags: Combination of sprite flags (spriteBottom, spriteCenter, etc.)
    foreign static sprite(spriteId, x, y, z, scale, rotation, mul, add, flags)

    /// Draws a shape at a position with transformation
    foreign static shape(shapeId, x, y, z, scale, rotation, mul, add)

    /// Draws text at a position with styling
    /// Note: Text always renders above sprites currently
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

    /// Primitive type for line rendering
    static lines { 0 }

    /// Primitive type for triangle rendering
    static triangles { 1 }

    /// Begins a debug primitive batch
    /// Call dbgVertex() to add vertices, then dbgEnd() to finish
    /// Primitive can be lines or triangles
    foreign static dbgBegin(primitive)

    /// Ends a debug primitive batch and renders it
    /// Number of vertices must match primitive type (divisible by 2 for lines, 3 for triangles)
    foreign static dbgEnd()

    /// Adds a vertex to the current debug primitive
    /// Must be called between dbgBegin() and dbgEnd()
    foreign static dbgVertex(x, y)

    /// Sets the color for the next debug vertices
    /// Color format: 0xRRGGBBAA (e.g., 0xF0C0D0FF)
    foreign static dbgColor(color)

    /// Draws a debug line from (x0, y0) to (x1, y1)
    foreign static dbgLine(x0, y0, x1, y1)

    /// Draws debug text on screen with specified size
    /// Uses built-in debug font
    foreign static dbgText(text, x, y, size)

    /// Draws a debug line between two vector points
    static dbgLine(a, b) {
        dbgLine(a.x, a.y, b.x, b.y)
    }

    /// Draws a filled debug rectangle
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

    /// Draws a filled debug square centered at position
    static dbgSquare(centerX, centerY, size) {
        var s = size * 0.5
        Render.dbgRect(centerX - s, centerY - s, centerX + s, centerY + s)
    }

    /// Draws a filled debug circle (disk)
    /// divs controls the number of triangular segments
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

    /// Draws a debug circle outline
    /// divs controls the number of line segments
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

    /// Draws a debug arc (partial circle outline)
    /// angle is in radians, divs controls line segment count
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

    /// Draws a filled debug pie/wedge shape
    /// angle is in radians, divs controls triangle count
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

    /// Adds a vector point as a debug vertex
    static dbgVertex(v) {
        Render.dbgVertex(v.x, v.y)
    }

    /// Draws a sprite at position with default settings
    /// Equivalent to: sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    static sprite(spriteId, x, y) {
        sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }

    /// Draws a sprite at position with z-sorting
    static sprite(spriteId, x, y, z) {
        sprite(spriteId, x, y, z, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }

    /// Draws a sprite at position with z-sorting and custom flags
    static sprite(spriteId, x, y, z, flags) {
        sprite(spriteId, x, y, z, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, flags)
    }

    /// Creates a sprite from a grid/sprite sheet by column and row
    static createGridSprite(imageId, columns, rows,  c, r) {
        var ds = 1 / columns
        var dt = 1 / rows        
        var s = c * ds
        var t = r * dt
        return createSprite(imageId, s, t, s + ds, t + dt)
    }

    /// Creates a sprite from a grid/sprite sheet by index
    /// Index starts at 0 from top-left, going row by row
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

/// File I/O operations
class File {
    /// Reads the contents of a file as a string
    foreign static read(src)

    /// Writes text content to a file
    foreign static write(text, dst)

    /// Checks if a file exists at the given path
    foreign static exists(src)
}

/// Data class for touch input information
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

/// Input handling for keyboard, mouse, gamepad, and touch
class Input {
    /// Gets the current value of a gamepad axis (-1.0 to 1.0)
    foreign static getAxis(axis)

    /// Gets axis value once when it crosses threshold (prevents repeating)
    foreign static getAxisOnce(axis, threshold)

    /// Checks if a gamepad button is currently pressed
    foreign static getButton(button)

    /// Checks if a gamepad button was just pressed (doesn't repeat while held)
    foreign static getButtonOnce(button)

    /// Checks if a keyboard key is currently pressed
    foreign static getKey(key)

    /// Checks if a keyboard key was just pressed (doesn't repeat while held)
    foreign static getKeyOnce(key)

    /// Gets mouse state information
    foreign static getMouse()

    /// Checks if a mouse button is currently pressed
    foreign static getMouseButton(button)

    /// Checks if a mouse button was just pressed (doesn't repeat while held)
    foreign static getMouseButtonOnce(button)

    /// Gets the current mouse X position in screen coordinates
    foreign static getMouseX()

    /// Gets the current mouse Y position in screen coordinates
    foreign static getMouseY()

    /// Gets the mouse wheel delta for this frame
    foreign static getMouseWheel()

    /// Gets the number of active touch points
    foreign static getNrTouches()

    /// Gets the unique ID for a touch at the given index
    foreign static getTouchId(index)

    /// Gets the X position for a touch at the given index
    foreign static getTouchX(index)

    /// Gets the Y position for a touch at the given index
    foreign static getTouchY(index)

    /// Sets gamepad vibration motors (DualSense, Xbox controllers)
    /// time is in milliseconds
    foreign static setPadVibration(lowRumble, highRumble, time)

    /// Sets the gamepad lightbar color (DualSense controller)
    /// Colors are 0-255
    foreign static setPadLightbarColor(red, green, blue)

    /// Resets gamepad lightbar to default color
    foreign static resetPadLightbarColor()

    /// Gets all touch data as a list of TouchData objects
    static getTouchData() {
        var nrTouches = getNrTouches()
        var result = []
        for (i in 0...nrTouches) result.add(getTouchData(i))
        return result
    }

    /// Gets touch data for a specific touch index
    static getTouchData(index) {
        return TouchData.new(getTouchId(index), getTouchX(index), getTouchY(index))
    }

    /// Gets the mouse position as a two-element list [x, y]
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
    static keyF11    { 300 }

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

/// Audio playback using FMOD
class Audio {

    /// Loads a sound file into a group and returns a sound ID
    foreign static load(name, groupId)

    /// Plays a loaded sound and returns a channel ID
    foreign static play(soundId)

    /// Gets the volume level of a sound group (0.0 to 1.0)
    foreign static getGroupVolume(groupId)

    /// Sets the volume level of a sound group (0.0 to 1.0)
    foreign static setGroupVolume(groupId, volume)

    /// Gets the volume level of a specific channel (0.0 to 1.0)
    foreign static getChannelVolume(channelId)

    /// Sets the volume level of a specific channel (0.0 to 1.0)
    foreign static setChannelVolume(channelId, volume)

    /// Gets the volume level of an FMOD bus by name (0.0 to 1.0)
    foreign static getBusVolume(busName)

    /// Sets the volume level of an FMOD bus by name (0.0 to 1.0)
    foreign static setBusVolume(busName, volume)

    /// Loads an FMOD sound bank
    foreign static loadBank(bankId)

    /// Unloads an FMOD sound bank
    foreign static unloadBank(bankId)

    /// Starts an FMOD event and returns an event instance ID
    foreign static startEvent(eventName)

    /// Sets a numeric parameter on an FMOD event instance
    foreign static setParameterNumber(eventId, paramName, newValue)

    /// Sets a labeled parameter on an FMOD event instance
    foreign static setParameterLabel(eventId, paramName, newValue)

    static groupSFX    { 1 }
    static groupMusic  { 2 }
}

class SimpleAudio {
    /// Load an audio file and return an audio id
    foreign static load(path)    
    
    /// Play a loaded audio file with specified volume (0.0 to 1.0)
    foreign static play(audioId, volume)

    /// Play a loaded audio file with default volume (1.0)
    static play(audioId) {
        return play(audioId, 1.0)
    }
    
    /// Set the volume of a playing channel (0.0 to 1.0)
    foreign static setVolume(channelId, volume)
    
    /// Get the volume of a playing channel
    foreign static getVolume(channelId)
    
    /// Stop a playing channel
    foreign static stop(channelId)
    
    /// Stop all playing channels
    foreign static stopAll()
    
    /// Check if a channel is currently playing
    foreign static isPlaying(channelId)
}

class Data {
    /// Gets a number value from the game scope
    static getNumber(name) { getNumber(name, game) }

    /// Gets a color value from the game scope
    static getColor(name)  { getColor(name, game) }

    /// Gets a boolean value from the game scope
    static getBool(name)  { getBool(name, game) }

    /// Gets a number value from a specific data scope
    foreign static getNumber(name, type)

    /// Gets a color value from a specific data scope
    foreign static getColor(name, type)

    /// Gets a boolean value from a specific data scope
    foreign static getBool(name, type)

    /// Gets a string value from a specific data scope
    foreign static getString(name, type)

    /// Sets a number value in a specific data scope
    foreign static setNumber(name, value, type)

    /// Sets a color value in a specific data scope
    foreign static setColor(name, value, type)

    /// Sets a boolean value in a specific data scope
    foreign static setBool(name, value, type)

    /// Sets a string value in a specific data scope
    foreign static setString(name, value, type)

    static system   { 2 }
    static debug    { 3 }
    static game     { 4 }
    static player   { 5 }
}

/// Platform and device information
class Device {

    /// Gets the current platform identifier
    foreign static getPlatform()

    /// Checks if the application can be closed (always false on consoles)
    foreign static canClose()

    /// Requests the application to close
    foreign static requestClose()
    foreign static setFullscreen(fullscreen)

    static PlatformPC      { 0 }
    static PlatformPS5     { 1 }
    static PlatformSwitch  { 2 }
}

/// CPU profiling utilities
class Profiler {
    /// Begins a named profiler section
    foreign static begin(name)

    /// Ends a named profiler section
    foreign static end(name)
}

/// ImGui-based inspector utilities for entity debugging
class Inspector {
    /// Displays text in the inspector
    foreign static text(label)

    /// Starts a collapsible tree node, returns true if open
    foreign static treeNode(label)

    /// Ends a tree node (must be called if treeNode returned true)
    foreign static treePop()

    /// Draws a horizontal separator line
    foreign static separator()

    /// Places the next widget on the same line
    foreign static sameLine()

    /// Increases indent level
    foreign static indent()

    /// Decreases indent level
    foreign static unindent()

    /// Adds vertical spacing
    foreign static spacing()
}
