import "xs"for Input
import "xs_ec"for Entity, Component
import "xs_components" for Sprite, Body
import "xs_math" for Color
import "tags" for Team

class Unit is Component {
    construct new(team, health, destroy) {
        super()
        _team = team
        _maxHealth = health
        _health = health        
        _destroy = destroy
        _timer = 0
    }

    initialize() {
        _sprite = owner.getComponentSuper(Sprite)
    }

    damage(d) {
        _health = _health - d
        _timer = 0.15

        if(_team == Team.Player) {
            Input.setPadVibration(10, 10)
        }
    }
    team { _team }
    health { _health }
    maxHealth { _maxHealth }

    update(dt) {
        if(_health <= 0) {
            _health = 0
            if(_destroy) {
                owner.delete()
            } 
            // Game.createExplosion(owner)
            return
        }
        
        if(_timer > 0) {
            _timer = _timer - dt
            _sprite.add = 0xE0E0E000
            if(_timer < 0) {
                _sprite.add = 0x00000000
            }
        }
    }

    toString { "[Unit team:%(_team) health:%(_health)]" }
}