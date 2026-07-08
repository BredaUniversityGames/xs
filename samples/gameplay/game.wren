import "xs/core" for Render, Data, Input
import "xs/math" for Math, Color
import "xs/tools" for ShapeBuilder
import "xs/ec" for Entity, Component
import "xs/components" for Transform, Body, Renderable, Sprite
import "background" for Background
import "shadow" for Shadow
import "trail" for Trail
import "debug" for DebugColor

class Game {
    static initialize() {
        Entity.initialize()        
        __time = 0
        __score = 0
        __font = Render.loadFont("[game]/assets/fonts/Amalgama.ttf", 16)
        __background = Background.new()

        // Create player
        __player = Create.player()
        
        // Create spawner entity
        var spawner = Entity.new()
        var spawnerComp = EnemySpawner.new(__player)
        spawner.add(spawnerComp)
        spawner.name = "Spawner"
        spawner.tag = Tag.spawner
        
        // Create collision system entity
        var collisionEntity = Entity.new()
        var collisionComp = CollisionSystem.new()

        // Register which tag pairs to test for overlaps
        collisionComp.addPair(Tag.attack, Tag.enemy)
        collisionComp.addPair(Tag.player, Tag.enemy, true)  // true = resolve overlap
        collisionComp.addPair(Tag.player, Tag.pickup)
        collisionComp.addPair(Tag.enemy, Tag.enemy, true)   // push enemies apart, no callback

        // Gameplay responses to each collision type
        collisionComp.on(Tag.attack, Tag.enemy) { |atk, enemy|
            if (atk.deleted || enemy.deleted) return
            var attackComp  = atk.get(MeleeAttack)
            var enemyHealth = enemy.get(Health)
            if (attackComp == null || enemyHealth == null) return
            if (!attackComp.canHit(enemy)) return
            attackComp.registerHit(enemy)
            enemyHealth.damage(attackComp.damage)
        }

        collisionComp.on(Tag.player, Tag.enemy) { |player, enemy|
            if (player.deleted || enemy.deleted) return
            var playerHealth = player.get(Health)
            if (playerHealth == null) return
            playerHealth.damage(Data.getNumber("Enemy Damage"))
            enemy.delete()
        }

        collisionComp.on(Tag.player, Tag.pickup) { |player, pickup|
            if (player.deleted || pickup.deleted) return
            var pickupComp = pickup.get(Pickup)
            if (pickupComp == null) return
            Game.addScore(pickupComp.value)
            pickup.delete()
        }

        collisionEntity.add(collisionComp)
        collisionEntity.name = "CollisionSystem"

        // Set to true to draw collision bodies as circles
        CollisionSystem.debug = true
        
        /*
        // Create a few obstacles
        for(i in 0...6) {
            var obstacle = Create.obstacle()
        }
        */
    }    

    static update(dt) {
        __time = __time + dt
        __background.update(dt)
        Entity.update(dt)
        
        // Check if player is dead
        var players = Entity.withTag(Tag.player)
        if (players.count == 0) {
            // Game over - could restart here
        }
    }

    static addScore(value) {
        __score = __score + value
    }

    static render() {
        __background.render()
        Renderable.render()
        Trail.render()
        Shadow.render()
        CollisionSystem.debugRender()

        // Draw small circle at mouse position for testing
        var mouseX = Input.getMouseX()
        var mouseY = Input.getMouseY()
        Render.dbgColor(0xff00ff00)  // Green color
        Render.dbgDisk(mouseX, mouseY, 5, 16)  // Small filled circle with 16 segments

        // Render UI
        var scoreText = "Score: %(__score)"
        Render.text(__font, scoreText, -620, 320, 10, 0xffffffff, 0x00000000, 0)

        // Render health if player exists
        var players = Entity.withTag(Tag.player)
        if (players.count > 0) {
            var player = players[0]
            var health = player.get(Health)
            if (health != null) {
                var healthText = "HP: %(health.health.floor)"
                Render.text(__font, healthText, -620, 200, 10, 0xffffffff, 0x00000000, 0)
            }
        }

        // Render enemy count
        var enemies = Entity.withTag(Tag.enemy)
        var enemyText = "Enemies: %(enemies.count)"
        Render.text(__font, enemyText, -620, 280, 10, 0xffffffff, 0x00000000, 0)

        if(Data.getBool("Debug.ShowPhysics", Data.game)) {
            for(e in Entity.entities) {
                var b = e.get(Body)
                if(b != null) {
                    var t = e.get(Transform)
                    var c = e.get(DebugColor)
                    Render.dbgColor(0xFF00FFFF)
                    if(c != null) {
                        Render.dbgColor(c.color)
                    }
                    Render.dbgDisk(t.position.x, t.position.y, b.size * 0.5, 24)
                }
            }
        }
    }
}

import "create" for Create
import "spawner" for EnemySpawner
import "collision" for CollisionSystem
import "health" for Health
import "attack" for MeleeAttack
import "pickup" for Pickup
import "tags" for Tag