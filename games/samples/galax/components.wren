import "xs" for Data
import "xs_ec" for Component
import "xs_components" for Transform
import "xs_math" for Vec2, Math

class SlowRelation is Component {
    construct new(parent, lambda) {        
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