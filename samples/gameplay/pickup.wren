import "xs/core" for Data
import "xs/ec" for Entity, Component
import "xs/math" for Vec2
import "xs/components" for Transform, Body

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
