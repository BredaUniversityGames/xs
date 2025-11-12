import "xs" for Data
import "xs_ec" for Entity, Component
import "xs_math" for Vec2, Bits
import "xs_components" for Transform, Body
import "random" for Random
import "create" for Create
import "tags" for Tag

// EnemySpawner - spawns enemies at intervals
class EnemySpawner is Component {
    construct new(player) {
        super()
        _player = player
        _spawnInterval = Data.getNumber("Enemy Spawn Interval")
        _spawnTimer = _spawnInterval
        _maxEnemies = Data.getNumber("Max Enemies")
        _random = Random.new()
    }

    update(dt) {
        _spawnTimer = _spawnTimer - dt
        
        if (_spawnTimer <= 0) {
            _spawnTimer = _spawnInterval
            
            // Check current enemy count
            var enemies = Entity.withTag(Tag.enemy)
            if (enemies.count < _maxEnemies) {
                spawnEnemy()
            }
        }
    }

    spawnEnemy() {
        var worldWidth = Data.getNumber("World Width")
        var worldHeight = Data.getNumber("World Height")
        
        // Spawn on the edges of the screen
        var side = _random.int(0, 3)
        var x = 0
        var y = 0
        
        if (side == 0) {
            // Top
            x = _random.float(-worldWidth * 0.5, worldWidth * 0.5)
            y = worldHeight * 0.5 + 50
        } else if (side == 1) {
            // Right
            x = worldWidth * 0.5 + 50
            y = _random.float(-worldHeight * 0.5, worldHeight * 0.5)
        } else if (side == 2) {
            // Bottom
            x = _random.float(-worldWidth * 0.5, worldWidth * 0.5)
            y = -worldHeight * 0.5 - 50
        } else {
            // Left
            x = -worldWidth * 0.5 - 50
            y = _random.float(-worldHeight * 0.5, worldHeight * 0.5)
        }
        
        Create.enemy(Vec2.new(x, y), _player)
    }

    toString { "[EnemySpawner]" }
}
