import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2
import "xs_components" for Transform

// Health component for entities that can take damage
class Health is Component {
    construct new(maxHealth) {
        super()
        _maxHealth = maxHealth
        _currentHealth = maxHealth
        _isDead = false
    }

    damage(amount) {
        _currentHealth = _currentHealth - amount
        if (_currentHealth <= 0) {
            _currentHealth = 0
            _isDead = true
            owner.delete()
        }
    }

    heal(amount) {
        _currentHealth = _currentHealth + amount
        if (_currentHealth > _maxHealth) {
            _currentHealth = _maxHealth
        }
    }

    isDead { _isDead }
    health { _currentHealth }
    maxHealth { _maxHealth }

    toString { "[Health current:%(_currentHealth) max:%(_maxHealth)]" }
}
