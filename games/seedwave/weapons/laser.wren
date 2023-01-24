import "xs" for Data
import "xs_ec" for Component

class Laser  is Component {
    construct new() {
        super()
        _time = 0        
    }

    update(dt) {
        _time = _time + dt
        if(_time > Data.getNumber("Laser Shoot Time")) {
            // System.print("Buzz")
            // Bullet.create(owner, -Data.getNumber("Cannon Round Speed"), 0)
            _time = 0
        }
    }
    
    shoot() {
    }
}