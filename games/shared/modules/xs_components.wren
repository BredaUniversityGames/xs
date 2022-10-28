import "xs" for Render
import "xs_ec"for Component, Entity
import "xs_math"for Vec2

class Transform is Component {
    construct new(position) {
         super()
        _position = position
    }

    position { _position }
    position=(p) { _position = p }

    toString { "[Transform position:%(_position)]" }
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
        Render.sprite(_sprite, t.position.x, t.position.y, layer, _scale, _rotation, _mul, _add, _flags)
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
        rotation = 0.0
        scale = 1.0
        mul = 0xFFFFFFFF        
        add = 0x00000000
        flags = 0
    }

    render() {
        var t = owner.getComponent(Transform)
        Render.text(_font, _text, t.position.x, t.position.y, mul, add, flags)
    }

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

    toString { "[GridSprite _idx:%(_idx) from:%(_sprites.count) ] -> " + super.toString }
}

class AnimatedSprite is GridSprite {
    construct new(image, columns, rows, fps) {
        super(image, columns, rows)
        _animations = {}
        _time = 0.0
        _flipFrames = (60.0 / fps).round
        _currentName = ""
        _currentFrame = 0
        _frame = 0
        _mode = AnimatedSprite.loop
    }

    update(dt) {
        if(_currentName == "") {
            return
        }

        var currentAnimation = _animations[_currentName]

        _frame = _frame + 1
        if(_frame >= _flipFrames) {
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
            _frame = 0
        }

        idx = currentAnimation[_currentFrame]
    }

    addAnimation(name, frames) {
        // assert name is string
        // assert frames is list
        _animations[name] = frames
    }

    playAnimation(name) {
        // assert name is string
        _currentFrame = 0
        _currentName = name
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
}

class Relation is Component {
    construct new(parent) {
        _parent = parent
        _offset = Vec2.new(0, 0)
    }

    update(dt) {
        var pt = _parent.getComponent(Transform)
        owner.getComponent(Transform).position = pt.position + _offset

        if(_parent.deleted) {
            owner.delete()
        }
    }

    offset { _offset }
    offset=(o) { _offset = o }

    toString { "[Relation parent:%(_parent) offset:%(_offset) ]" }
}
