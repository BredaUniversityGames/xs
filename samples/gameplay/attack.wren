import "xs/core" for Data
import "xs/ec" for Entity, Component

// Short-lived melee hitbox. Persists for one swing duration then self-destructs.
// Tracks which enemies have already been hit so each is damaged at most once per swing.
class MeleeAttack is Component {
construct new(damage, lifetime) {
        super()
        _damage   = damage
        _lifetime = lifetime
        _time     = 0
        _hit      = []
    }

    update(dt) {
        _time = _time + dt
        if (_time >= _lifetime) {
            owner.delete()
        }
    }

    // Returns false if this entity was already struck during this swing
    canHit(entity) {
        for (e in _hit) {
            if (e == entity) return false
        }
        return true
    }

    registerHit(entity) {
        _hit.add(entity)
    }

    damage { _damage }

    toString { "[MeleeAttack damage:%(_damage)]" }
}
