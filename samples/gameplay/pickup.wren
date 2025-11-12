import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2
import "xs_components" for Transform, Body

// Pickup component - collectible that increases score
class Pickup is Component {
    construct new(value) {
        super()
        _value = value
        _lifetime = Data.getNumber("Pickup Lifetime")
        _time = 0
    }

    update(dt) {
        _time = _time + dt
        
        // Fade out and destroy after lifetime
        if (_time >= _lifetime) {
            owner.delete()
        }
    }

    value { _value }

    toString { "[Pickup value:%(_value)]" }
}
