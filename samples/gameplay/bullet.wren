import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2, Math
import "xs_components" for Transform, Body

// Bullet component - damages enemies on collision
class Bullet is Component {
    construct new(direction, speed) {
        super()
        _direction = direction.normalized
        _speed = speed
        _damage = Data.getNumber("Bullet Damage")
        _lifetime = Data.getNumber("Bullet Lifetime")
        _time = 0
    }

    initialize() {
        _transform = owner.get(Transform)
        _body = owner.get(Body)
        
        // Set initial velocity
        _body.velocity = _direction * _speed
        
        // Rotate to face movement direction
        _transform.rotation = _direction.atan2
    }

    update(dt) {
        _time = _time + dt
        
        // Destroy bullet after lifetime expires
        if (_time >= _lifetime) {
            owner.delete()
        }
    }

    damage { _damage }

    toString { "[Bullet damage:%(_damage)]" }
}
