import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2
import "xs_components" for Transform, Body
import "tags" for Tag

// Enemy component - chases the player with obstacle avoidance
class Enemy is Component {
    construct new(target) {
        super()
        _target = target
        _speed = Data.getNumber("Enemy Speed")
        _avoidanceRadius = Data.getNumber("Enemy Avoidance Radius")
        _avoidanceStrength = Data.getNumber("Enemy Avoidance Strength")
    }

    initialize() {
        _transform = owner.get(Transform)
        _body = owner.get(Body)
    }

    update(dt) {
        if (_target == null || _target.deleted) {
            return
        }

        var targetTransform = _target.get(Transform)
        if (targetTransform == null) {
            return
        }

        // Calculate direction to player
        var direction = targetTransform.position - _transform.position
        var distance = direction.magnitude
        
        if (distance > 0.1) {
            direction = direction.normal
            
            // Apply obstacle avoidance
            var avoidance = calculateObstacleAvoidance()
            direction = (direction + avoidance * _avoidanceStrength).normal
            
            _body.velocity = direction * _speed
            
            // Rotate to face movement direction
            _transform.rotation = direction.atan2
        }
    }

    calculateObstacleAvoidance() {
        var avoidance = Vec2.new(0, 0)
        var obstacles = Entity.withTag(Tag.obstacle)
        
        for (obstacle in obstacles) {
            var obstacleTransform = obstacle.get(Transform)
            var obstacleBody = obstacle.get(Body)
            
            if (obstacleTransform == null || obstacleBody == null) {
                continue
            }
            
            var toObstacle = obstacleTransform.position - _transform.position
            var distance = toObstacle.magnitude
            var combinedRadius = _avoidanceRadius + obstacleBody.size * 0.5
            
            // If within avoidance radius, add repulsion force
            if (distance < combinedRadius && distance > 0.1) {
                var repulsion = toObstacle.normal * -1
                var strength = 1.0 - (distance / combinedRadius)
                avoidance = avoidance + repulsion * strength
            }
        }
        
        return avoidance
    }

    toString { "[Enemy]" }
}

