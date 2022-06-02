import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "components" for Transform

class Orbitor is Component {

    construct new(parent) {
        super()
        _period = 2.0           
        _time = 0.0
        _tilt = 0.3
        _a = 30.0
        _b = 70.0
        _ships = []
        _parent = parent
    }

    update(dt) {
        var speed = Num.pi * 2.0 / _period
        _time = _time + dt * speed

        if(_ships.count > 0) {
            var delta = Num.pi * 2.0 / _ships.count
            var t = _time
            for(i in 0..._ships.count) {
                var ship = _ships[i]
                if(ship != null && !ship.deleted) {
                    var a = t + delta * i
                    var relPosition = Vec2.new(_a * a.sin, _b * a.cos)
                    relPosition.rotate(_tilt)
                    var parPosition = _parent.getComponent(Transform).position
                    ship.getComponent(Transform).position = relPosition + parPosition
                }
                t = t + delta
            }
        }
    }

    del() {
        for(ship in _ships) {
            if(ship != null) {
                ship.delete()
            }
        }
    }

    add(ship) {
        _ships.add(ship)
    }

    remove(ship) {
        _ships.remove(ship)
    }

    nullify(ship) {
        for(i in 0..._ships.count) {
            var s = _ships[i]
            if(s != null && s == ship) {
                _ships[i] = null
                // System.print("found")
            }
        }
    }

    trim() {
        while(true) {
            var found = false
            for(i in 0..._ships.count) {            
                if(_ships[i].deleted) {
                    _ships.removeAt(i)
                    found = true
                    break
                }
            }
            if(found == false) {
                break
            }
        }
    }
}

class Shield is Component {
    update(dt) {}
}
