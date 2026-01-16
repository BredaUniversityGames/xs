import "xs/core" for Data
import "xs/ec" for Entity, Component
import "xs/math" for Vec2
import "xs/components" for Transform

// Health component for entities that can take damage
class Health is Component {
    construct new(maxHealth) {
        super()
        _maxHealth = maxHealth
        _currentHealth = maxHealth
        _isDead = false
        _dropPickupOnDeath = false
        _pickupValue = 0
    }

    // Enable dropping a pickup when this entity dies
    enablePickupDrop(value) {
        _dropPickupOnDeath = true
        _pickupValue = value
    }

    damage(amount) {
        _currentHealth = _currentHealth - amount
        if (_currentHealth <= 0) {
            _currentHealth = 0
            _isDead = true
            
            // Drop pickup if enabled
            if (_dropPickupOnDeath) {
                var transform = owner.get(Transform)
                if (transform != null) {
                    Create.pickup(transform.position, _pickupValue)
                }
            }
            
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

import "create" for Create
