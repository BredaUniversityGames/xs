import "xs_ec"for Entity, Component
import "xs_components" for Sprite

class Unit is Component {
    construct new(team, health) {
        super()
        _team = team
        _health = health
        _timer = 0
    }

    damage(d) {
        _health = _health - d
        _timer = 0.15
    }
    team { _team }
    health { _health }

    update(dt) {
        if(_health <= 0) {
            Game.createExplosion(owner)
            owner.delete()
            return
        }
        
        if(_timer > 0) {
            _timer = _timer - dt
            var s = owner.getComponentSuper(Sprite)
            s.add = 0xE0E0E000
            if(_timer < 0) {
                s.add = 0x00000000
            }
        }
    }

    toString { "[Unit team:%(_team) health:%(_health)]" }
}


import "game" for Game
import "tags" for Team