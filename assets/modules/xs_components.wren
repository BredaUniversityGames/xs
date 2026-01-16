import "xs" for Render, File
import "xs_ec"for Component, Entity
import "xs_math"for Vec2
import "external/json" for JSON

/// Component that stores position and rotation for an entity
class Transform is Component {
    /// Creates a new Transform at the given position with zero rotation
    construct new(position) {
         super()
        _position = position
        _rotation = 0.0
    }

    /// Gets the position as a Vec2
    #!inspect(type = "vec2")
    position { _position }
    /// Sets the position
    #!inspect
    position=(p) { _position = p }

    /// Gets the rotation in radians
    #!inspect(type = "angle")
    rotation { _rotation }
    /// Sets the rotation in radians
    #!inspect
    rotation=(r) { _rotation = r }

    toString { "[Transform position:%(_position) rotation:%(_rotation)]" }
}

/// Component that provides size and velocity for physics-based movement
class Body is Component {
    /// Creates a new Body with the given size and velocity
    construct new(size, velocity) {
        super()
        _scale = size
        _velocity = velocity
    }

    /// Gets the size of the body
    #!inspect(type = "vec2")
    size { _scale }
    /// Gets the velocity as a Vec2
    #!inspect(type = "vec2")
    velocity { _velocity }

    /// Sets the size of the body
    #!inspect
    size=(s) { _scale = s }
    /// Sets the velocity
    #!inspect
    velocity=(v) { _velocity = v }

    /// Updates the position based on velocity and delta time
    update(dt) {
        var t = owner.get(Transform)
        t.position = t.position + _velocity * dt
    }

    toString { "[Body velocity:%(_velocity) size:%(_scale)]" }
}

/// Base class for all renderable components with layer-based sorting
class Renderable is Component {
    /// Creates a new Renderable with default layer 0
    construct new() {
        super()
        _layer = 0.0
    }

    /// Override this method in subclasses to implement rendering
    render() {}

    /// Comparison operator for sorting by layer
    <(other) {
        layer  < other.layer
    }

    /// Gets the rendering layer (higher values render on top)
    #!inspect(type = "number")
    layer { _layer }
    /// Sets the rendering layer
    #!inspect
    layer=(l) { _layer = l }

    /// Renders all Renderable components in all entities
    static render() {        
        for(e in Entity.entities) {
            var s = e.get(Renderable)
            if(s != null) {
                s.render()                
            }
        }
    }

    toString { "[Renderable layer:%(_layer)]" }
}

/// Renders a sprite from an image or texture atlas
class Sprite is Renderable {
    /// Creates a sprite from a full image (path or image ID)
    construct new(image) {
        super()
        if(image is String) {
            image = Render.loadImage(image)
        }
        _sprite = Render.createSprite(image, 0, 0, 1, 1)
        _rotation = 0.0
        _scale = 1.0
        _mul = 0xFFFFFFFF        
        _add = 0x00000000
        _flags = 0
    }

    /// Creates a sprite from a section of an image using normalized texture coordinates
    construct new(image, s0, t0, s1, t1) {
        super()
        if(image is String) {
            image = Render.loadImage(image)
        }
        _sprite = Render.createSprite(image, s0, t0, s1, t1)
        _rotation = 0.0
        _scale = 1.0
        _mul = 0xFFFFFFFF
        _add = 0x00000000
        _flags = 0
    }

    /// Creates a sprite from a spritesheet file (.xssprite) by sprite name
    /// The spritesheet file contains JSON with an "image" path and "sprites" definitions
    /// Each sprite has x, y, width, and height in pixels
    construct new(sheetPath, spriteName) {
        super()

        // Read and parse the spritesheet file (with caching)
        var data = JSON.load(sheetPath)

        // Load the image
        var imagePath = data["image"]
        var imageId = Render.loadImage(imagePath)

        // Get the sprite definition
        var sprites = data["sprites"]
        var spriteData = sprites[spriteName]

        // Get image dimensions for normalizing coordinates
        var imageWidth = Render.getImageWidth(imageId)
        var imageHeight = Render.getImageHeight(imageId)

        // Convert pixel coordinates to normalized texture coordinates (0.0 to 1.0)
        var x = spriteData["x"]
        var y = spriteData["y"]
        var width = spriteData["width"]
        var height = spriteData["height"]

        var s0 = x / imageWidth
        var t0 = y / imageHeight
        var s1 = (x + width) / imageWidth
        var t1 = (y + height) / imageHeight
        
        _sprite = Render.createSprite(imageId, s0, t0, s1, t1)
        _rotation = 0.0
        _scale = 1.0
        _mul = 0xFFFFFFFF
        _add = 0x00000000
        _flags = 0
    }

    /// Initializes by caching reference to Transform component
    initialize() {
        _transform = owner.get(Transform)
    }

    /// Renders the sprite at the Transform's position and rotation
    render() {        
        Render.sprite(
            _sprite,
            _transform.position.x,
            _transform.position.y,
            layer,
            _scale,            
            _transform.rotation,
            _mul,
            _add,
            _flags)
    }

    /// Gets the additive color (0xRRGGBBAA format)
    #!inspect(type = "color")
    add { _add }
    /// Sets the additive color
    #!inspect
    add=(a) { _add = a }

    /// Gets the multiply color (0xRRGGBBAA format)
    #!inspect(type = "color")
    mul { _mul }
    /// Sets the multiply color
    #!inspect
    mul=(m) { _mul = m }

    /// Gets the sprite flags (flipping, centering, etc.)
    #!inspect(type = "number")
    flags { _flags }
    /// Sets the sprite flags
    #!inspect
    flags=(f) { _flags = f }

    /// Gets the sprite scale
    #!inspect(type = "number")
    scale { _scale }
    /// Sets the sprite scale
    #!inspect
    scale=(s) { _scale = s }

    /// Sets the sprite ID
    sprite_=(s) { _sprite = s }
    /// Gets the sprite ID
    #!inspect(type = "number")
    sprite { _sprite }

    toString { "[Sprite sprite:%(_sprite)] -> " + super.toString }
}

/// Renders a shape (SVG or custom mesh)
class Shape is Renderable {
    /// Creates a Shape component with the given shape ID
    construct new(shape) {
        super()
        _shape = shape
        _rotation = 0.0
        _scale = 1.0
        _mul = 0xFFFFFFFF        
        _add = 0x00000000
        _flags = 0
    }

    /// Renders the shape at the Transform's position and rotation
    render() {
        var t = owner.get(Transform)
        Render.sprite(
            _shape,
            t.position.x,
            t.position.y,
            layer,
            _scale,            
            t.rotation,
            _mul,
            _add,
            _flags)
    }

    /// Gets the additive color (0xRRGGBBAA format)
    add { _add }
    /// Sets the additive color
    add=(a) { _add = a }

    /// Gets the multiply color (0xRRGGBBAA format)
    mul { _mul }
    /// Sets the multiply color
    mul=(m) { _mul = m }

    /// Gets the shape flags
    flags { _flags }
    /// Sets the shape flags
    flags=(f) { _flags = f }

    /// Gets the shape scale
    scale { _scale }
    /// Sets the shape scale
    scale=(s) { _scale = s }

    /// Sets the shape ID
    shape=(s) { _shape = s }
    /// Gets the shape ID
    shape { _shape }

    toString { "[Shape shape:%(_shape)] -> " + super.toString }
}

/// Renders text using a loaded font
class Label is Sprite {
    /// Creates a Label with the given font (path or font ID), text content, and size
    construct new(font, text, size) {
        super()
        if(font is String) {
            font = Render.loadFont(font, size)
        }
        _font = font
        _text = text
        sprite_ = null
        scale = 1.0
        mul = 0xFFFFFFFF        
        add = 0x00000000
        flags = 0
    }

    /// Renders the text at the Transform's position
    render() {
        var t = owner.get(Transform)
        Render.text(_font, _text, t.position.x, t.position.y, layer, mul, add, flags)
    }

    /// Gets the text content
    text { _text }
    /// Sets the text content
    text=(t) { _text = t }
}

/// Sprite that can display frames from a sprite sheet grid
class GridSprite is Sprite {
    /// Creates a GridSprite from an image divided into columns and rows
    construct new(image, columns, rows) {
        super(image, 0.0, 0.0, 1.0, 1.0)
        if(image is String) {
            image = Render.loadImage(image)
        }

        // assert columns or rows should be above one

        _sprites = []
        var ds = 1 / columns
        var dt = 1 / rows        
        for(j in 0...rows) {
            for(i in 0...columns) {
                var s = i * ds
                var t = j * dt
                _sprites.add(Render.createSprite(image, s, t, s + ds, t + dt))
            }
        }
        
        _idx = 0
        sprite_ = _sprites[_idx]
    }

    /// Sets the current frame index
    idx=(i) {
        _idx = i
        sprite_ = _sprites[_idx]
    }

    /// Gets the current frame index
    idx{ _idx }

    /// Gets the sprite ID at the given frame index
    [i] { _sprites[i] }

    toString { "[GridSprite _idx:%(_idx) from:%(_sprites.count) ] -> " + super.toString }
}

/// Sprite with animation support for playing frame sequences from a sprite sheet
/// Use addAnimation() to define named animations, then playAnimation() to start them
/// Call update(dt) each frame to advance the animation
class AnimatedSprite is GridSprite {
    /// Creates an AnimatedSprite with the given frame rate (frames per second)
    /// fps determines how fast animations play - higher values = faster animations
    construct new(image, columns, rows, fps) {
        super(image, columns, rows)
        _animations = {}
        _time = 0.0
        _flipFrames = 1.0 / (fps + 1)
        _frameTime = 0.0
        _currentName = ""
        _currentFrame = 0
        // _frame = 0
        _mode = AnimatedSprite.loop
    }

    /// Updates the animation frame based on delta time
    update(dt) {
        if(_currentName == "") {
            return
        }

        var currentAnimation = _animations[_currentName]

        _frameTime = _frameTime + dt

        if(_frameTime >= _flipFrames) {
            if(_mode == AnimatedSprite.once) {
                _currentFrame = (_currentFrame + 1)
                if(_currentFrame >= currentAnimation.count) {
                    _currentFrame = currentAnimation.count - 1
                }
            } else if(_mode == AnimatedSprite.loop) {
                _currentFrame = (_currentFrame + 1) % currentAnimation.count
            } else if (_mode == AnimatedSprite.destroy) {
                _currentFrame = _currentFrame + 1
                if(_currentFrame == currentAnimation.count) {
                    owner.delete()
                    return
                }
            }
            _frameTime = 0.0
        }

        idx = currentAnimation[_currentFrame]
    }

    /// Adds a named animation with a list of frame indices
    /// Frame indices correspond to positions in the sprite sheet (0-indexed, left to right, top to bottom)
    /// Example: addAnimation("walk", [0, 1, 2, 3])
    addAnimation(name, frames) {
        // TODO: assert name is string
        // TODO: assert frames is list
        _animations[name] = frames
    }

    /// Plays the animation with the given name, restarting from the first frame
    /// Does nothing if the animation name doesn't exist
    playAnimation(name) {
        if(_animations.containsKey(name)) {
            _currentFrame = 0
            _currentName = name
        }
    }

    /// Randomizes the current frame within the current animation
    randomizeFrame(random) {
        _currentFrame = random.int(0, _animations[_currentName].count)
    }

    /// Gets the animation mode
    mode { _mode }
    /// Sets the animation mode (once, loop, or destroy)
    mode=(m) { _mode = m }
    /// Checks if animation has finished (for non-looping animations)
    isDone { _mode != AnimatedSprite.loop && _currentFrame == _animations[_currentName].count - 1}

    /// Play animation once and stop on the last frame
    static once { 0 }
    /// Loop animation continuously (default behavior)
    static loop { 1 }
    /// Delete the owning entity when animation completes (useful for effects)
    static destroy { 2 }    

    toString { "[AnimatedSprite _mode:%(_mode) _currentName:%(_currentName) ] -> " + super.toString }
}

/// Makes an entity follow its parent's position with an offset
class Relation is Component {
    /// Creates a Relation that follows the given parent entity
    construct new(parent) {
        super()
        _parent = parent
        _offset = Vec2.new(0, 0)
    }

    /// Updates position to follow parent (with rotation support)
    update(dt) {
        var pt = _parent.get(Transform)
        var offset = _offset
        if(pt.rotation != 0.0) {
            offset = _offset.rotated(pt.rotation)
        }
        owner.get(Transform).position = pt.position + offset 

        if(_parent.deleted) {
            owner.delete()
        }
    }

    /// Gets the offset from parent position
    offset { _offset }
    /// Sets the offset from parent position
    offset=(o) { _offset = o }
    /// Gets the parent entity
    parent { _parent }

    toString { "[Relation parent:%(_parent) offset:%(_offset) ]" }
}

/// Deletes this entity when its parent is deleted
class Ownership is Component {
    /// Creates an Ownership component tied to the given parent entity
    construct new(parent) {
        super()
        _parent = parent
    }

    /// Checks if parent is deleted and deletes this entity if so
    update(dt) {
        if(_parent.deleted) {
            owner.delete()
        }
    }

    /// Gets the parent entity
    parent { _parent }

    toString { "[Ownership parent:%(_parent) ]" }
}

