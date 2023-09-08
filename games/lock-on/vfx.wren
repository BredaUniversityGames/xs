import "xs" for Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class SpriteParticle {
    construct new(sprite, pos, vel, fromSize, toSize, life) {
        _sprite = sprite
        _pos = pos
        _vel = vel
        _life = 0.0
        _normLife = 1.0 / life
        _fromSize = fromSize
        _toSize = toSize
        _size = fromSize
    }

    update(dt) {
        _pos = _pos + _vel * dt
        _life = _life + dt * _normLife

        if (_life >= 0.0) {
            _size = _fromSize + (_toSize - _fromSize) * _life
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
            0.0,
            0xFFFFFFFF,
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

        /*
        for (i in 0..._particles.count) {
            var p = _particles[i]
            p.update(dt)
            if(p.live() == false) {
                _particles.removeAt(i)
                i = i - 1
            }
        }
        */

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
    construct new(particleSystem, sprite) {
        _particleSystem = particleSystem
        _sprite = sprite
    }

    update(dt) {
        var pos = owner.getComponent(Transform).position
        var vel = Vec2.new(-100.0, 0.0)
        var size = 1.0        
        var p = SpriteParticle.new(_sprite, pos, vel, size, 0.0, 0.6)
        _particleSystem.add(p)
    }
}