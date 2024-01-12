import "xs" for Render
import "xs_ec"for Component, Entity
import "xs_math"for Vec2

class Transform is Component {
    construct new(position) {
         super()
        _position = position
        _rotation = 0.0
    }

    position { _position }
    position=(p) { _position = p }

    rotation { _rotation }
    rotation=(r) { _rotation = r }

    toString { "[Transform position:%(_position) rotation:%(_rotation)]" }
}

class Body is Component {    
    construct new(size, velocity) {
        super()
        _scale = size
        _velocity = velocity
    }

    size { _scale }
    velocity { _velocity }

    size=(s) { _scale = s }
    velocity=(v) { _velocity = v }

    update(dt) {
        var t = owner.getComponent(Transform)
        t.position = t.position + _velocity * dt
    }

    toString { "[Body velocity:%(_velocity) size:%(_scale)]" }
}

class Renderable is Component {
    construct new() {
        _layer = 0.0
    }

    render() {}

    <(other) {
        layer  < other.layer
    }

    layer { _layer }
    layer=(l) { _layer = l }

    static render() {        
        for(e in Entity.entities) {
            var s = e.getComponentSuper(Renderable)
            if(s != null) {
                s.render()                
            }
        }
    }

    toString { "[Renderable layer:%(_layer)]" }
}

class Sprite is Renderable {
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

    render() {        
        var t = owner.getComponent(Transform)
        Render.sprite(
            _sprite,
            t.position.x,
            t.position.y,
            layer,
            _scale,            
            t.rotation,         // _rotation,
            _mul,
            _add,
            _flags)
    }

    add { _add }
    add=(a) { _add = a }

    mul { _mul }
    mul=(m) { _mul = m }

    flags { _flags }
    flags=(f) { _flags = f }

    scale { _scale }
    scale=(s) { _scale = s }

    rotation { _rotation }
    rotation=(r) { _rotation = r }

    sprite_=(s) { _sprite = s }
    sprite { _sprite }

    toString { "[Sprite sprite:%(_sprite)] -> " + super.toString }
}

class Label is Sprite {
    construct new(font, text, size) {
        super()
        if(font is String) {
            font = Render.loadFont(font, size)
        }
        _font = font
        _text = text
        sprite_ = null
        // rotation = 0.0
        scale = 1.0
        mul = 0xFFFFFFFF        
        add = 0x00000000
        flags = 0
    }

    render() {
        var t = owner.getComponent(Transform)
        Render.text(_font, _text, t.position.x, t.position.y, mul, add, flags)
    }

    text { _text }
    text=(t) { _text = t }

    //toString { "[Sprite sprite:%(_sprite)] -> " + super.toString }
}

class GridSprite is Sprite {
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

    idx=(i) {
        _idx = i
        sprite_ = _sprites[_idx]
    }

    idx{ _idx }

    [i] { _sprites[i] }

    toString { "[GridSprite _idx:%(_idx) from:%(_sprites.count) ] -> " + super.toString }
}

class AnimatedSprite is GridSprite {
    construct new(image, columns, rows, fps) {
        super(image, columns, rows)
        _animations = {}
        _time = 0.0
        _flipFrames = 1.0 / fps
        _frameTime = 0.0
        _currentName = ""
        _currentFrame = 0
        // _frame = 0
        _mode = AnimatedSprite.loop        
    }

    update(dt) {
        if(_currentName == "") {
            return
        }

        var currentAnimation = _animations[_currentName]

        // _frame = _frame + 1

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

    addAnimation(name, frames) {
        // assert name is string
        // assert frames is list
        _animations[name] = frames
    }

    playAnimation(name) {
        if(_animations.containsKey(name)) {
            _currentFrame = 0
            _currentName = name
        }
    }

    randomizeFrame(random) {        
        _currentFrame = random.int(0, _animations[_currentName].count)
    }

    mode { _mode }
    mode=(m) { _mode = m }
    isDone { _mode != AnimatedSprite.loop && _currentFrame == _animations[_currentName].count - 1}

    static once { 0 }
    static loop { 1 } 
    static destroy { 2 }    

    toString { "[AnimatedSprite _mode:%(_mode) _currentName:%(_currentName) ] -> " + super.toString }
}

class Relation is Component {
    construct new(parent) {
        _parent = parent
        _offset = Vec2.new(0, 0)
    }

    update(dt) {
        var pt = _parent.getComponent(Transform)
        var offset = _offset
        if(pt.rotation != 0.0) {
            offset = _offset.rotated(pt.rotation)
        }
        owner.getComponent(Transform).position = pt.position + offset 

        if(_parent.deleted) {
            owner.delete()
        }
    }

    offset { _offset }
    offset=(o) { _offset = o }
    parent { _parent }

    toString { "[Relation parent:%(_parent) offset:%(_offset) ]" }
}

class Ownership is Component {
    construct new(parent) {
        _parent = parent
    }

    update(dt) {    
        if(_parent.deleted) {
            owner.delete()
        }
    }

    parent { _parent }

    toString { "[Ownership parent:%(_parent) ]" }
}

