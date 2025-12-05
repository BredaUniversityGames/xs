import "xs" for Render, Data, Input
import "xs_math" for Math, Color
import "xs_tools" for ShapeBuilder
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Body, Renderable, Sprite
import "background" for Background
import "shadow" for Shadow

class Game {
    static initialize() {
        Entity.initialize()        
        __time = 0
        __score = 0
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 40)
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
        collisionEntity.add(collisionComp)
        collisionEntity.name = "CollisionSystem"
        
        // Create a few obstacles
        for(i in 0...6) {
            var obstacle = Create.obstacle()
        }
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
        Shadow.render()

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
                Render.text(__font, healthText, -620, 280, 10, 0xffffffff, 0x00000000, 0)
            }
        }

        // Render enemy count
        var enemies = Entity.withTag(Tag.enemy)
        var enemyText = "Enemies: %(enemies.count)"
        Render.text(__font, enemyText, -620, 240, 10, 0xffffffff, 0x00000000, 0)
    }
}

import "create" for Create
import "spawner" for EnemySpawner
import "collision" for CollisionSystem
import "health" for Health
import "tags" for Tag