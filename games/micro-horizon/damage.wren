import "xs_ec"for Entity, Component

class Damage is Component {
    construct new(damage) {
        super()
        _damage = damage   
    }

    damage { _damage }
}
