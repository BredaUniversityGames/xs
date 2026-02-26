import "xs/math" for Vec2

/// Handle for shapes and sprites in the rendering system.
/// Used to correctly manage resources via the GC. Not to be created directly.
foreign class ShapeHandle {}

/// Core rendering API for sprites, shapes, text, and debug drawing
/// Provides functionality for rendering images, texts and shapes
class Render {

    /// Loads an image from a file and returns an image ID
    /// Supports PNG and JPG formats. Use relative paths like "[game]/textures/flower.png"
    foreign static loadImage(path: String) -> Num

    /// Creates a procedural image from pixel data and returns an image ID
    /// pixels is a flat list of width*height packed RGBA values (use ImageBuilder.pack())
    foreign static createImage(width: Num, height: Num, pixels: List) -> Num

    /// Loads a shape from a file and returns a shape ID
    /// Supports SVG format
    foreign static loadShape(path: String) -> Num

    /// Loads a font into a font atlas and returns a font ID
    /// Font will be rasterized at the specified size
    foreign static loadFont(font: String, size: Num) -> Num

    /// Gets the width in pixels of a loaded image
    foreign static getImageWidth(imageId: Num) -> Num

    /// Gets the height in pixels of a loaded image
    foreign static getImageHeight(imageId: Num) -> Num

    /// Creates a sprite from a section of an image using texture coordinates
    /// Coordinates are normalized (0.0 to 1.0): x0, y0 (top-left), x1, y1 (bottom-right)
    foreign static createSprite(imageId: Num, x0: Num, y0: Num, x1: Num, y1: Num) -> Num

    /// Creates a custom mesh shape from vertices, texture coordinates, and indices
    foreign static createShape(imageId: Num, positions: List, textureCoords: List, indices: List) -> Num

    /// Destroys a shape and frees its resources
    foreign static destroyShape(shapeId: Num)

    /// Sets the offset for subsequent sprite draw calls
    /// All sprites will be offset by (x, y) until new values are set
    foreign static setOffset(x: Num, y: Num)

    /// Draws a sprite with full control over appearance
    /// - spriteId: Valid sprite ID created with createSprite (not an image ID)
    /// - x, y: Position on screen (affected by setOffset)
    /// - z: Sorting depth value
    /// - scale: Scaling factor
    /// - rotation: Rotation angle in radians
    /// - mul: Multiply color (0xRRGGBBAA format)
    /// - add: Additive color (0xRRGGBBAA format)
    /// - flags: Combination of sprite flags (spriteBottom, spriteCenter, etc.)
    foreign static sprite(spriteId: Num, x: Num, y: Num, z: Num, scale: Num, rotation: Num, mul: Num, add: Num, flags: Num)

    /// Draws a shape at a position with transformation
    foreign static shape(shapeId: Num, x: Num, y: Num, z: Num, scale: Num, rotation: Num, mul: Num, add: Num)

    /// Draws text at a position with styling
    /// Note: Text always renders above sprites currently
    foreign static text(fontId: Num, txt: String, x: Num, y: Num, z: Num, mul: Num, add: Num, flags: Num)
    
    /// Don't apply any flags
    static spriteNone -> Num { 0 << 0 }

    /// Draw the sprite at the bottom
    static spriteBottom -> Num { 1 << 1 }

    /// Draw the sprite at the top
    static spriteTop -> Num { 1 << 2 }

    /// Center the sprite on the x-axis
    static spriteCenterX -> Num { 1 << 3 }

    /// Center the sprite on the y-axis
    static spriteCenterY -> Num { 1 << 4 }

    /// Flip the sprite on the x-axis
    static spriteFlipX -> Num { 1 << 5 }

    /// Flip the sprite on the y-axis
    static spriteFlipY -> Num { 1 << 6 }

    /// Overlay the sprite as overlay (no offset applied)
    static spriteFixed -> Num { 1 << 7 }

    /// Center the sprite on the x and y-axis
    static spriteCenter -> Num { spriteCenterX | spriteCenterY }

    /// This is not a sprite but a shape, so handle it differently
    static spriteShape -> Num { 1 << 8 }

    /// Primitive type for line rendering
    static lines -> Num { 0 }

    /// Primitive type for triangle rendering
    static triangles -> Num { 1 }

    /// Begins a debug primitive batch
    /// Call dbgVertex() to add vertices, then dbgEnd() to finish
    /// Primitive can be lines or triangles
    foreign static dbgBegin(primitive: Num)

    /// Ends a debug primitive batch and renders it
    /// Number of vertices must match primitive type (divisible by 2 for lines, 3 for triangles)
    foreign static dbgEnd()

    /// Adds a vertex to the current debug primitive
    /// Must be called between dbgBegin() and dbgEnd()
    foreign static dbgVertex(x: Num, y: Num)

    /// Sets the color for the next debug vertices
    /// Color format: 0xRRGGBBAA (e.g., 0xF0C0D0FF)
    foreign static dbgColor(color: Num)

    /// Draws a debug line from (x0, y0) to (x1, y1)
    foreign static dbgLine(x0: Num, y0: Num, x1: Num, y1: Num)

    /// Draws debug text on screen with specified size
    /// Uses built-in debug font
    foreign static dbgText(text: String, x: Num, y: Num, size: Num)

    /// Draws a debug line between two vector points
    static dbgLine(a: Vec2, b: Vec2) {
        dbgLine(a.x, a.y, b.x, b.y)
    }

    /// Draws a filled debug rectangle
    static dbgRect(fromX: Num, fromY: Num, toX: Num, toY: Num) {
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
    static dbgSquare(centerX: Num, centerY: Num, size: Num) {
        var s = size * 0.5
        Render.dbgRect(centerX - s, centerY - s, centerX + s, centerY + s)
    }

    /// Draws a filled debug circle (disk)
    /// divs controls the number of triangular segments
    static dbgDisk(x: Num, y: Num, r: Num, divs: Num) {
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
    static dbgCircle(x: Num, y: Num, r: Num, divs: Num) {
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
    static dbgArc(x: Num, y: Num, r: Num, angle: Num, divs: Num) {
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
    static dbgPie(x: Num, y: Num, r: Num, angle: Num, divs: Num) {
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
    static dbgVertex(v: Vec2) {
        Render.dbgVertex(v.x, v.y)
    }

    /// Draws a sprite at position with default settings
    /// Equivalent to: sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    static sprite(spriteId: Num, x: Num, y: Num) {
        sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }

    /// Draws a sprite at position with z-sorting
    static sprite(spriteId: Num, x: Num, y: Num, z: Num) {
        sprite(spriteId, x, y, z, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)
    }

    /// Draws a sprite at position with z-sorting and custom flags
    static sprite(spriteId: Num, x: Num, y: Num, z: Num, flags: Num) {
        sprite(spriteId, x, y, z, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, flags)
    }

    /// Creates a sprite from a grid/sprite sheet by column and row
    static createGridSprite(imageId: Num, columns: Num, rows: Num, c: Num, r: Num) -> Num {
        var ds = 1 / columns
        var dt = 1 / rows
        var s = c * ds
        var t = r * dt
        return createSprite(imageId, s, t, s + ds, t + dt)
    }

    /// Creates a sprite from a grid/sprite sheet by index
    /// Index starts at 0 from top-left, going row by row
    static createGridSprite(imageId: Num, columns: Num, rows: Num, idx: Num) -> Num {
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
    foreign static read(src: String) -> String

    /// Writes text content to a file
    foreign static write(text: String, dst: String)

    /// Checks if a file exists at the given path
    foreign static exists(src: String) -> Bool
}

/// Fast JSON parsing and serialization (C++ implementation)
/// Much faster than the pure Wren json.wren parser
class Json {
    /// Loads and parses a JSON file, returns the parsed value (Map, List, String, Num, Bool, or null)
    /// Returns null if the file doesn't exist or parsing fails
    foreign static load(path: String)

    /// Parses a JSON string, returns the parsed value
    /// Returns null if parsing fails
    foreign static parse(jsonString: String)

    /// Saves a value to a JSON file with pretty formatting
    /// Returns true on success, false on failure
    /// Note: Map serialization has limited support
    foreign static save(path: String, value) -> Bool

    /// Converts a value to a JSON string with pretty formatting
    /// Note: Map serialization has limited support
    foreign static stringify(value) -> String
}

/// Data class for touch input information
class TouchData {
    construct new(index: Num, x: Num, y: Num) {
        _index = index
        _x = x
        _y = y
    }

    index -> Num { _index }
    x -> Num { _x }
    y -> Num { _y }
}

/// Input handling for keyboard, mouse, gamepad, and touch
class Input {
    /// Gets the current value of a gamepad axis (-1.0 to 1.0)
    foreign static getAxis(axis: Num) -> Num

    /// Gets axis value once when it crosses threshold (prevents repeating)
    foreign static getAxisOnce(axis: Num, threshold: Num) -> Num

    /// Checks if a gamepad button is currently pressed
    foreign static getButton(button: Num) -> Bool

    /// Checks if a gamepad button was just pressed (doesn't repeat while held)
    foreign static getButtonOnce(button: Num) -> Bool

    /// Checks if a keyboard key is currently pressed
    foreign static getKey(key: Num) -> Bool

    /// Checks if a keyboard key was just pressed (doesn't repeat while held)
    foreign static getKeyOnce(key: Num) -> Bool

    /// Gets mouse state information
    foreign static getMouse()

    /// Checks if a mouse button is currently pressed
    foreign static getMouseButton(button: Num) -> Bool

    /// Checks if a mouse button was just pressed (doesn't repeat while held)
    foreign static getMouseButtonOnce(button: Num) -> Bool

    /// Gets the current mouse X position in screen coordinates
    foreign static getMouseX() -> Num

    /// Gets the current mouse Y position in screen coordinates
    foreign static getMouseY() -> Num

    /// Gets the mouse wheel delta for this frame
    foreign static getMouseWheel() -> Num

    /// Gets the number of active touch points
    foreign static getNrTouches() -> Num

    /// Gets the unique ID for a touch at the given index
    foreign static getTouchId(index: Num) -> Num

    /// Gets the X position for a touch at the given index
    foreign static getTouchX(index: Num) -> Num

    /// Gets the Y position for a touch at the given index
    foreign static getTouchY(index: Num) -> Num

    /// Sets gamepad vibration motors (DualSense, Xbox controllers)
    /// time is in milliseconds
    foreign static setPadVibration(lowRumble: Num, highRumble: Num, time: Num)

    /// Sets the gamepad lightbar color (DualSense controller)
    /// Colors are 0-255
    foreign static setPadLightbarColor(red: Num, green: Num, blue: Num)

    /// Resets gamepad lightbar to default color
    foreign static resetPadLightbarColor()

    /// Gets all touch data as a list of TouchData objects
    static getTouchData() -> List {
        var nrTouches = getNrTouches()
        var result = []
        for (i in 0...nrTouches) result.add(getTouchData(i))
        return result
    }

    /// Gets touch data for a specific touch index
    static getTouchData(index: Num) -> TouchData {
        return TouchData.new(getTouchId(index), getTouchX(index), getTouchY(index))
    }

    /// Gets the mouse position as a two-element list [x, y]
    static getMousePosition() -> List {
        return [getMouseX(), getMouseY()]
    }

    static keyRight -> Num { 262 }
    static keyLeft -> Num { 263 }
    static keyDown -> Num { 264 }
    static keyUp -> Num { 265 }
    static keySpace -> Num { 32  }
    static keyEscape -> Num { 256 }
    static keyEnter -> Num { 257 }
    static keyF11 -> Num { 300 }

    static keyA -> Num { 65 }
    static keyB -> Num { 66 }
    static keyC -> Num { 67 }
    static keyD -> Num { 68 }
    static keyE -> Num { 69 }
    static keyF -> Num { 70 }
    static keyG -> Num { 71 }
    static keyH -> Num { 72 }
    static keyI -> Num { 73 }
    static keyJ -> Num { 74 }
    static keyK -> Num { 75 }
    static keyL -> Num { 76 }
    static keyM -> Num { 77 }
    static keyN -> Num { 78 }
    static keyO -> Num { 79 }
    static keyP -> Num { 80 }
    static keyQ -> Num { 81 }
    static keyR -> Num { 82 }
    static keyS -> Num { 83 }
    static keyT -> Num { 84 }
    static keyU -> Num { 85 }
    static keyV -> Num { 86 }
    static keyW -> Num { 87 }
    static keyX -> Num { 88 }
    static keyY -> Num { 89 }
    static keyZ -> Num { 90 }

    static gamepadButtonSouth -> Num { 0  }
    static gamepadButtonEast -> Num { 1  }
    static gamepadButtonWest -> Num { 2  }
    static gamepadButtonNorth -> Num { 3  }
    static gamepadShoulderLeft -> Num { 4  }
    static gamepadShoulderRight -> Num { 5  }
    static gamepadButtonSelect -> Num { 6  }
    static gamepadButtonStart -> Num { 6  }

    static gamepadLeftStickPress -> Num { 9  }
    static gamepadRightStickPress -> Num { 10 }
    static gamepadDPadUp -> Num { 11 }
    static gamepadDPadDown -> Num { 12 }
    static gamepadDPadLeft -> Num { 13 }
    static gamepadDPadRight -> Num { 14 }

    static gamepadAxisLeftStickX -> Num { 0  }
    static gamepadAxisLeftStickY -> Num { 1  }
    static gamepadAxisRightStickX -> Num { 2  }
    static gamepadAxisRightStickY -> Num { 3  }
    static gamepadAxisLeftTrigger -> Num { 4  }
    static gamepadAxisRightTrigger -> Num { 5  }

    static mouseButtonLeft -> Num { 0 }
    static mouseButtonRight -> Num { 1 }
    static mouseButtonMiddle -> Num { 2 }
}

/// Audio playback using FMOD
class Audio {

    /// Loads a sound file into a group and returns a sound ID
    foreign static load(name: String, groupId: Num) -> Num

    /// Plays a loaded sound and returns a channel ID
    foreign static play(soundId: Num) -> Num

    /// Gets the volume level of a sound group (0.0 to 1.0)
    foreign static getGroupVolume(groupId: Num) -> Num

    /// Sets the volume level of a sound group (0.0 to 1.0)
    foreign static setGroupVolume(groupId: Num, volume: Num)

    /// Gets the volume level of a specific channel (0.0 to 1.0)
    foreign static getChannelVolume(channelId: Num) -> Num

    /// Sets the volume level of a specific channel (0.0 to 1.0)
    foreign static setChannelVolume(channelId: Num, volume: Num)

    /// Gets the volume level of an FMOD bus by name (0.0 to 1.0)
    foreign static getBusVolume(busName: String) -> Num

    /// Sets the volume level of an FMOD bus by name (0.0 to 1.0)
    foreign static setBusVolume(busName: String, volume: Num)

    /// Loads an FMOD sound bank
    foreign static loadBank(bankId: String)

    /// Unloads an FMOD sound bank
    foreign static unloadBank(bankId: String)

    /// Starts an FMOD event and returns an event instance ID
    foreign static startEvent(eventName: String) -> Num

    /// Sets a numeric parameter on an FMOD event instance
    foreign static setParameterNumber(eventId: Num, paramName: String, newValue: Num)

    /// Sets a labeled parameter on an FMOD event instance
    foreign static setParameterLabel(eventId: Num, paramName: String, newValue: String)

    static groupSFX -> Num { 1 }
    static groupMusic -> Num { 2 }
}

class SimpleAudio {
    /// Load an audio file and return an audio id
    foreign static load(path: String) -> Num

    /// Play a loaded audio file with specified volume (0.0 to 1.0)
    foreign static play(audioId: Num, volume: Num) -> Num

    /// Play a loaded audio file with default volume (1.0)
    static play(audioId: Num) -> Num {
        return play(audioId, 1.0)
    }

    /// Set the volume of a playing channel (0.0 to 1.0)
    foreign static setVolume(channelId: Num, volume: Num)

    /// Get the volume of a playing channel
    foreign static getVolume(channelId: Num) -> Num

    /// Stop a playing channel
    foreign static stop(channelId: Num)

    /// Stop all playing channels
    foreign static stopAll()

    /// Check if a channel is currently playing
    foreign static isPlaying(channelId: Num) -> Bool
}

class Data {
    /// Gets a number value from the game scope
    static getNumber(name: String) -> Num { getNumber(name, game) }

    /// Gets a color value from the game scope
    static getColor(name: String) -> Num { getColor(name, game) }

    /// Gets a boolean value from the game scope
    static getBool(name: String) -> Bool { getBool(name, game) }

    /// Gets a number value from a specific data scope
    foreign static getNumber(name: String, type: Num) -> Num

    /// Gets a color value from a specific data scope
    foreign static getColor(name: String, type: Num) -> Num

    /// Gets a boolean value from a specific data scope
    foreign static getBool(name: String, type: Num) -> Bool

    /// Gets a string value from a specific data scope
    foreign static getString(name: String, type: Num) -> String

    /// Sets a number value in a specific data scope
    foreign static setNumber(name: String, value: Num, type: Num)

    /// Sets a color value in a specific data scope
    foreign static setColor(name: String, value: Num, type: Num)

    /// Sets a boolean value in a specific data scope
    foreign static setBool(name: String, value: Bool, type: Num)

    /// Sets a string value in a specific data scope
    foreign static setString(name: String, value: String, type: Num)

    static system -> Num { 2 }
    static debug -> Num { 3 }
    static game -> Num { 4 }
    static player -> Num { 5 }
}

/// Platform and device information
class Device {

    /// Gets the current platform identifier
    foreign static getPlatform() -> Num

    /// Checks if the application can be closed (always false on consoles)
    foreign static canClose() -> Bool

    /// Requests the application to close
    foreign static requestClose()
    foreign static setFullscreen(fullscreen: Bool)

    static PlatformPC -> Num { 0 }
    static PlatformPS5 -> Num { 1 }
    static PlatformSwitch -> Num { 2 }
}

/// CPU profiling utilities
class Profiler {
    /// Begins a named profiler section
    foreign static begin(name: String)

    /// Ends a named profiler section
    foreign static end(name: String)
}

/// ImGui-based inspector utilities for entity debugging
class Inspector {
    /// Displays text in the inspector
    foreign static text(label: String)

    /// Starts a collapsible tree node, returns true if open
    foreign static treeNode(label: String) -> Bool

    /// Ends a tree node (must be called if treeNode returned true)
    foreign static treePop()

    /// Draws a horizontal separator line
    foreign static separator()

    /// Draws a horizontal separator line with text in the middle
    foreign static separatorText(text: String)

    /// Places the next widget on the same line
    foreign static sameLine()

    /// Increases indent level
    foreign static indent()

    /// Decreases indent level
    foreign static unindent()

    /// Adds vertical spacing
    foreign static spacing()

    /// Selectable item for lists, returns true if clicked
    foreign static selectable(label: String, selected: Bool) -> Bool

    /// Input field for float values, returns new value
    foreign static inputFloat(label: String, value: Num) -> Num

    /// Drag widget for float values, returns new value
    foreign static dragFloat(label: String, value: Num) -> Num

    /// Checkbox for boolean values, returns new value
    foreign static checkbox(label: String, value: Bool) -> Bool

    /// Creates a collapsing header section (cleaner than treeNode for headers)
    /// Returns true if the section is open/expanded
    foreign static collapsingHeader(label: String) -> Bool

    /// Begins a child window region with optional border
    /// width and height: size in pixels (0 = auto-size)
    /// border: whether to draw a border around the child
    foreign static beginChild(label: String, width: Num, height: Num, border: Bool)

    /// Ends the current child window region
    /// Must be called after beginChild
    foreign static endChild()

    /// Private: Drag widget for 2D float vector (x, y as separate params)
    /// Returns a list [x, y] with the new values
    foreign static dragFloat2_(label: String, x: Num, y: Num) -> List

    /// Drag widget for 2D vector, returns new Vec2 value
    /// Works with xs_math Vec2 objects
    static dragFloat2(label: String, vec: Vec2) -> Vec2 {
        var result = dragFloat2_(label, vec.x, vec.y)
        vec.x = result[0]
        vec.y = result[1]
        return vec
    }
}
