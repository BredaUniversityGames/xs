import "xs/core" for Data
import "xs/ec" for Entity, Component
import "xs/math" for Vec2, Bits
import "xs/components" for Transform, Body
import "tags" for Tag
import "health" for Health
import "bullet" for Bullet
import "pickup" for Pickup
import "game" for Game

// Collision system - checks for collisions between entities
class CollisionSystem is Component {
    construct new() {
        super()
    }

    update(dt) {
        checkBulletEnemyCollisions()
        checkBulletObstacleCollisions()
        checkPlayerEnemyCollisions()
        checkPlayerPickupCollisions()
    }

    checkBulletEnemyCollisions() {
        var bullets = Entity.withTag(Tag.bullet)
        var enemies = Entity.withTag(Tag.enemy)
        
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
                
                var distance = (bulletTransform.position - enemyTransform.position).magnitude
                var collisionRadius = bulletBody.size * 0.5 + enemyBody.size * 0.5
                
                if (distance < collisionRadius) {
                    enemyHealth.damage(bulletComp.damage)
                    bullet.delete()
                    break
                }
            }
        }
    }

    checkBulletObstacleCollisions() {
        var bullets = Entity.withTag(Tag.bullet)
        var obstacles = Entity.withTag(Tag.obstacle)
        
        for (bullet in bullets) {
            if (bullet.deleted) {
                continue
            }
            
            var bulletTransform = bullet.get(Transform)
            var bulletBody = bullet.get(Body)
            
            if (bulletTransform == null || bulletBody == null) {
                continue
            }
            
            for (obstacle in obstacles) {
                var obstacleTransform = obstacle.get(Transform)
                var obstacleBody = obstacle.get(Body)
                
                if (obstacleTransform == null || obstacleBody == null) {
                    continue
                }
                
                var distance = (bulletTransform.position - obstacleTransform.position).magnitude
                var collisionRadius = bulletBody.size * 0.5 + obstacleBody.size * 0.5
                
                if (distance < collisionRadius) {
                    bullet.delete()
                    break
                }
            }
        }
    }

    checkPlayerEnemyCollisions() {
        var players = Entity.withTag(Tag.player)
        var enemies = Entity.withTag(Tag.enemy)
        
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
                        var damage = Data.getNumber("Enemy Damage")
                        playerHealth.damage(damage)
                        enemy.delete()
                    }
                }
            }
        }
    }

    checkPlayerPickupCollisions() {
        var players = Entity.withTag(Tag.player)
        var pickups = Entity.withTag(Tag.pickup)
        
        if (players.count > 0) {
            var player = players[0]
            var playerTransform = player.get(Transform)
            var playerBody = player.get(Body)
            
            if (playerTransform != null && playerBody != null) {
                for (pickup in pickups) {
                    if (pickup.deleted) {
                        continue
                    }
                    
                    var pickupTransform = pickup.get(Transform)
                    var pickupBody = pickup.get(Body)
                    var pickupComp = pickup.get(Pickup)
                    
                    if (pickupTransform == null || pickupBody == null || pickupComp == null) {
                        continue
                    }
                    
                    var distance = (playerTransform.position - pickupTransform.position).magnitude
                    var collisionRadius = playerBody.size * 0.5 + pickupBody.size * 0.5
                    
                    if (distance < collisionRadius) {
                        // Player collected pickup!
                        Game.addScore(pickupComp.value)
                        pickup.delete()
                    }
                }
            }
        }
    }

    toString { "[CollisionSystem]" }
}

