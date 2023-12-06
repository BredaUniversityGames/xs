import "xs" for Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class SpriteParticle {
    construct new(sprite, pos, vel, fromSize, toSize, fromColor, toColor, angle, life) {
        _sprite = sprite
        _pos = pos
        _vel = vel
        _life = 0.0
        _normLife = 1.0 / life
        _fromSize = fromSize
        _toSize = toSize
        _size = fromSize
        _fromColor = fromColor
        _toColor = toColor
        _color = fromColor
        _angle = angle
    }

    update(dt) {
        _pos = _pos + _vel * dt        
        _life = _life + dt * _normLife

        if (_life >= 0.0) {
            _size = _fromSize + (_toSize - _fromSize) * _life
            _color = _fromColor + (_toColor - _fromColor) * _life
        }
    }

    live() {
        return _life < 1.0
    }

    render() {
        Render.sprite(
            _sprite,
            _pos.x,
            _pos.y,
            0.0,
            _size,            
            _angle,
            _color.toNum,
            0x00000000,
            Render.spriteCenter)
    }
}

class ParticleSystem {
    construct new() {
        _particles = []        
    }

    update(dt) {
        if (_particles.count == 0) {
            return    
        }

        var i = 0
        while (i < _particles.count) {
            var p = _particles[i]
            p.update(dt)
            if(p.live() == false) {
                _particles.removeAt(i)
            } else {
                i = i + 1
            }
        }
    }

    render() {
        for (p in _particles) {
            p.render()
        }
    }

    add(p) {
        _particles.add(p)
    }
}

class ParticleTrail is Component {
    construct new(particleSystem, sprite, offset) {
        _particleSystem = particleSystem
        _sprite = sprite
        _offset = offset
        //_lastPos = owner.getComponent(Transform).position + _offset
        _lastPos = Vec2.new(0,0)
    }

    update(dt) {
        var transform = owner.getComponent(Transform)
        var angle = transform.rotation + Math.pi / 2.0
        var offset = _offset.rotated(angle)
        var pos = transform.position + offset
        // var vel = (pos - _lastPos) / dt
        //_lastPos = pos        
        var fromSize = 1.5
        var toSize = 0.5
        var fromColor = Color.fromNum(0xFFFFFFFF)
        var toColor = Color.fromNum(0xFFFFFF00)
        var p = SpriteParticle.new( _sprite, pos, Vec2.new(0,0),
                                    fromSize, toSize,                                    
                                    fromColor, toColor,
                                    angle, 0.6)
        _particleSystem.add(p)
    }
}