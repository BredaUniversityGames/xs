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
        _multiplier = 1.0
        _damage = 0.0
    }

    initialize() {
        _sprite = owner.getComponentSuper(Sprite)
    }

    damage(d) {
        _health = _health - d * _multiplier
        _timer = 0.15
        _damage = d * _multiplier

        if(_team == Team.Player) {
            Input.setPadVibration(10, 10)
        }
    }
    team { _team }
    health { _health }
    maxHealth { _maxHealth }
    multiplier { _multiplier }
    multiplier=(m) { _multiplier = m }

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

    damage {
        var d = _damage
        _damage = 0.0
        return d
    }

    toString { "[Unit team:%(_team) health:%(_health)]" }
}