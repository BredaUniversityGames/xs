import "xs_ec"for Component

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