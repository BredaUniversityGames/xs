import "xs" for Render
import "xs_ec"for Component, Entity


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
        _size = size
        _velocity = velocity
    }

    size { _size }
    velocity { _velocity }

    size=(s) { _size = s }
    velocity=(v) { _velocity = v }

    update(dt) {
        var t = owner.getComponent(Transform)
        t.position = t.position + _velocity * dt
    }

    toString { "[Body velocity:%(_velocity) size:%(_size)]" }
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
        var sprites = []
        for(e in Entity.entities) {
            var s = e.getComponentSuper(Renderable)
            if(s != null) {
                sprites.add(s)                
            }
        }

        if(sprites.count != 0) {
            sprites.sort{|a, b|  a.layer < b.layer }
            for(s in sprites) {
                s.render()
            }
        }
    }
}

class Sprite is Renderable {
    construct new(image) {
        super()
        if(image is String) {
            image = Render.loadImage(image)
        }
        _sprite = Render.createSprite(image, 0, 0, 1, 1)
        _anchor = Render.anchorBottom
    }

    construct new(image, s0, t0, s1, t1) {
        super()
        if(image is String) {
            image = Render.loadImage(image)
        }
        _sprite = Render.createSprite(image, s0, t0, s1, t1)
        _anchor = Render.anchorBottom
    }

    render() {        
        var t = owner.getComponent(Transform)
        Render.renderSprite(_sprite, t.position.x, t.position.y, _anchor)
    }

    anchor { _anchor }
    anchor=(a) { _anchor = a }

    sprite_=(s) { _sprite = s }
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
        for(i in 0...columns) {
            for(j in 0...rows) {
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

        // assert currentAnimation not empty
        //_time = _time + dt
        //if( (_time - (1.0 /_fps)).abs < 0.01 ) {
            //_time = 0.0
            //_currentFrame = (_currentFrame + 1) % currentAnimation.count
        //}

        _frame = _frame + 1
        if(_frame >= _flipFrames) {
            if(_mode == AnimatedSprite.loop) {
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
        _currentName = name
    }

    mode { _mode }
    mode=(m) { _mode = m }

    static loop { 0 } 
    static destroy { 1 }
}
