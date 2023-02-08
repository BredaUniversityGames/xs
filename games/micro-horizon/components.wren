import "xs" for Data
import "xs_ec" for Component
import "xs_components" for Transform, Body
import "xs_math" for Vec2, Math

class SlowRelation is Component {
    construct new(parent, lambda) {   
        super()     
        _parent = parent
        _lambda = lambda
        _offset = Vec2.new(0, 0)
    }

    update(dt) {
        var pt = _parent.getComponent(Transform)                
        var target = pt.position + _offset
        owner.getComponent(Transform).position = Math.damp(owner.getComponent(Transform).position, target, _lambda, dt)
            //Damp(owner.getComponent(Transform).position, target, _lambda, dt)

        if(_parent.deleted) {
            owner.delete()
        }
    }

    offset { _offset }
    offset=(o) { _offset = o }

    toString { "[Relation parent:%(_parent) offset:%(_offset) ]" }
}

class DeleteOffBounds is Component {
        construct new() {
        super()
    }

    initialize() {
        _transform = owner.getComponent(Transform)
    }

    update(dt) {
        var t = _transform
        var w = Data.getNumber("Width", Data.system) * 0.52
        var h = Data.getNumber("Height", Data.system) * 0.52

        if (t.position.x < -w) {
            owner.delete()
        } else if (t.position.x > w) {
            owner.delete()
        }
        
        if (t.position.y < -h) {
            owner.delete()
        } else if (t.position.y > h) {
            owner.delete()
        }
    }
}

class TurnToVelocity is Component {
    construct new() { super() }

    initialize() {
        _body = owner.getComponent(Body)
        _transform = owner.getComponent(Transform)
    }

    update(dt) {
        if(_body.velocity.magnitude > 0.01) {
            var alpha = _body.velocity.atan2
            owner.getComponent(Transform).rotation = alpha
        }
    }

    toString { "[TurnToVelocity]" }
}

class TNB is Component {
    construct new() {
        super()
    }

    initialize() {
        _body = owner.getComponent(Body)
        _transform = owner.getComponent(Transform)
    }

    body { _body }
    transform { _transform }

    position { _transform.position }
    position=(p) { _transform.position = p }
    size { _body.size }
    rotation { _transform.rotation }
    rotation=(r) { _transform.rotation = r }

    
    toString { "[TNB body:%(_body) transform:%(_transform) i:%(initialized_) ]" }
}