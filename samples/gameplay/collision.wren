import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2, Bits
import "xs_components" for Transform, Body
import "tags" for Tag
import "health" for Health
import "bullet" for Bullet

// Collision system - checks for collisions between bullets and enemies
class CollisionSystem is Component {
    construct new() {
        super()
    }

    update(dt) {
        // Get all bullets and enemies
        var bullets = Entity.withTag(Tag.bullet)
        var enemies = Entity.withTag(Tag.enemy)
        
        // Check collisions
        for (bullet in bullets) {
            if (bullet.deleted) {
                continue
            }
            
            var bulletTransform = bullet.get(Transform)
            var bulletBody = bullet.get(Body)
            var bulletComp = bullet.get(Bullet)
            
            if (bulletTransform == null || bulletBody == null || bulletComp == null) {
                continue
            }
            
            for (enemy in enemies) {
                if (enemy.deleted) {
                    continue
                }
                
                var enemyTransform = enemy.get(Transform)
                var enemyBody = enemy.get(Body)
                var enemyHealth = enemy.get(Health)
                
                if (enemyTransform == null || enemyBody == null || enemyHealth == null) {
                    continue
                }
                
                // Simple circle collision
                var distance = (bulletTransform.position - enemyTransform.position).magnitude
                var collisionRadius = bulletBody.size * 0.5 + enemyBody.size * 0.5
                
                if (distance < collisionRadius) {
                    // Collision detected!
                    enemyHealth.damage(bulletComp.damage)
                    bullet.delete()
                    break
                }
            }
        }
        
        // Check player-enemy collisions
        var players = Entity.withTag(Tag.player)
        if (players.count > 0) {
            var player = players[0]
            var playerTransform = player.get(Transform)
            var playerBody = player.get(Body)
            var playerHealth = player.get(Health)
            
            if (playerTransform != null && playerBody != null && playerHealth != null) {
                for (enemy in enemies) {
                    if (enemy.deleted) {
                        continue
                    }
                    
                    var enemyTransform = enemy.get(Transform)
                    var enemyBody = enemy.get(Body)
                    
                    if (enemyTransform == null || enemyBody == null) {
                        continue
                    }
                    
                    var distance = (playerTransform.position - enemyTransform.position).magnitude
                    var collisionRadius = playerBody.size * 0.5 + enemyBody.size * 0.5
                    
                    if (distance < collisionRadius) {
                        // Player hit by enemy!
                        var damage = Data.getNumber("Enemy Damage")
                        playerHealth.damage(damage)
                        enemy.delete()
                    }
                }
            }
        }
    }

    toString { "[CollisionSystem]" }
}
