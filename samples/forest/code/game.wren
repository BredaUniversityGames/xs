import "xs/core" for Render, Input
import "xs/math" for Vec2
import "random" for Random

// Cycles through a list of sprite IDs at a fixed frame rate
class Animation {
    construct new(sprites) {
        _sprites = sprites
        _time = 0
        _frame = 0
    }

    update(dt) {
        _time = _time + dt
        if (_time > 1/6) {
            _time = _time - 1/6
            _frame = (_frame + 1) % _sprites.count
        }
    }

    current { _sprites[_frame] }
}

// Creates sprites from a tileset image using a tile index and tile pixel dimensions.
// Columns are derived from the image width at runtime.
class TileSheet {
    static create(imageId, index, tileW, tileH) {
        var imgW = Render.getImageWidth(imageId)
        var imgH = Render.getImageHeight(imageId)
        var cols = (imgW / tileW).truncate
        var rows = (imgH / tileH).truncate
        return Render.createGridSprite(imageId, cols, rows, index)
    }

    static createRange(imageId, from, to, tileW, tileH) {
        var sprites = []
        for (i in from..to) {
            sprites.add(TileSheet.create(imageId, i, tileW, tileH))
        }
        return sprites
    }
}

class Character {
    construct new() {
        var image = Render.loadImage("[game]/assets/playerSprites_/fPlayer_ [human].png")
        _idleAnim = Animation.new(TileSheet.createRange(image, 8, 11, 32, 32))
        _walkAnim = Animation.new(TileSheet.createRange(image, 16, 23, 32, 32))
        _pos = Vec2.new(0, 0)
        _speed = 50
        _walking = false
        _facingLeft = false
    }

    position { _pos }

    update(dt) {
        var axisX = Input.getAxis(0)
        var axisY = -Input.getAxis(1)

        var dx = 0
        var dy = 0

        if (axisX.abs > 0.1 || axisY.abs > 0.1) {
            dx = axisX
            dy = axisY
        } else {
            if (Input.getKey(Input.keyLeft)  || Input.getKey(Input.keyA)) dx = -1
            if (Input.getKey(Input.keyRight) || Input.getKey(Input.keyD)) dx =  1
            if (Input.getKey(Input.keyUp)    || Input.getKey(Input.keyW)) dy =  1
            if (Input.getKey(Input.keyDown)  || Input.getKey(Input.keyS)) dy = -1
        }

        _walking = dx != 0 || dy != 0

        if (_walking) {
            var dir = Vec2.new(dx, dy)
            _pos = _pos + dir.normal * _speed * dt
        }

        if (dx < 0) _facingLeft = true
        if (dx > 0) _facingLeft = false

        var anim = _walking ? _walkAnim : _idleAnim
        anim.update(dt)
    }

    render() {
        var flags = _facingLeft ? Render.spriteFlipX : Render.spriteNone
        var anim = _walking ? _walkAnim : _idleAnim
        Render.sprite(anim.current, _pos.x, _pos.y, -_pos.y + 0.2, flags)
    }
}

class Game {
    static initialize() {
        __groundSprites = []
        __propSprites = []
        __largePropSprites = []

        var groundImage = Render.loadImage("[game]/assets/forest_/forest_.png")
        for (id in [30, 31, 32, 33, 52, 53, 54, 55, 76, 77]) {
            __groundSprites.add(TileSheet.create(groundImage, id, 16, 16))
        }

        var propsImage = Render.loadImage("[game]/assets/forest_/forest_ [resources].png")
        for (id in [6, 7, 8, 9, 20, 30, 23, 33, 24, 34, 25, 35, 26, 36, 27, 37, 28, 38, 29, 39]) {
            __propSprites.add(TileSheet.create(propsImage, id, 16, 16))
        }
        for (id in [0, 1, 5]) {
            __largePropSprites.add(TileSheet.create(propsImage, id, 16, 32))
        }

        __player = Character.new()
        __camera = Vec2.new(0, 0)
    }

    static update(dt) {
        __player.update(dt)
        __camera = __player.position
    }

    static render() {
        var tileSize = 16
        var camTileX = (__camera.x / tileSize).round
        var camTileY = (__camera.y / tileSize).round

        Render.setOffset(-__camera.x.round, -__camera.y.round)

        for (tx in (camTileX - 9)..(camTileX + 8)) {
            for (ty in (camTileY - 9)..(camTileY + 8)) {
                var wx = tx * tileSize
                var wy = ty * tileSize

                // Seed RNG per tile so the world is deterministic
                var rng = Random.new((wx * 73856093 + wy * 19349663).abs.truncate)

                Render.sprite(__groundSprites[rng.int(__groundSprites.count)], wx, wy, -wy - tileSize)

                var roll = rng.float()
                if (roll < 0.20) {
                    Render.sprite(__largePropSprites[rng.int(__largePropSprites.count)], wx, wy, -wy + 0.1)
                } else if (roll < 0.55) {
                    Render.sprite(__propSprites[rng.int(__propSprites.count)], wx, wy, -wy + 0.1)
                }
            }
        }

        __player.render()
    }
}
