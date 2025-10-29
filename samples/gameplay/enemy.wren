import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2
import "xs_components" for Transform, Body

// Enemy component - chases the player
class Enemy is Component {
    construct new(target) {
        super()
        _target = target
        _speed = Data.getNumber("Enemy Speed")
    }

    initialize() {
        _transform = owner.get(Transform)
        _body = owner.get(Body)
    }

    update(dt) {
        if (_target == null || _target.deleted) {
            return
        }

        var targetTransform = _target.get(Transform)
        if (targetTransform == null) {
            return
        }

        // Calculate direction to player
        var direction = targetTransform.position - _transform.position
        var distance = direction.magnitude
        
        if (distance > 0.1) {
            direction = direction.normalized
            _body.velocity = direction * _speed
            
            // Rotate to face movement direction
            _transform.rotation = direction.atan2
        }
    }

    toString { "[Enemy]" }
}
