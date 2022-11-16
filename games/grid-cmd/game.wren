import "xs" for Render, Data, Input
import "xs_math" for Bits, Vec2, Math
import "grid" for Grid
import "random" for Random 

class Type {
    static none     { 0 << 0 }
    static player   { 1 << 0 }
    static enemy    { 1 << 1 }
    static bomb     { 1 << 2 }
    static wall     { 1 << 3 }
    static door     { 1 << 4 }
    static blocking { wall | door }
}

class Turn {
    static none     { 0 }
    static player   { 1 }
    static enemy    { 2 }
    static dead     { 3 }
}

class Game {

    static config() {
        Data.setString("Title", "Grid Commander V0.1a", Data.system)
        Data.setNumber("Width", 320, Data.system)
        Data.setNumber("Height", 180, Data.system)
        Data.setNumber("Multiplier", 4, Data.system)
    }

    static init() {        
        __time = 0 
        __turn = Turn.player
        __level = 3
        __rand = Random.new()

        
        __clear = Render.createSprite(Render.loadImage("[game]/white.png"), 0, 0, 1, 1)
        __gridTiles = Render.createSprite(Render.loadImage("[game]/tile_small_wall.png"), 0, 0, 1, 1)
        __bugSprite = Render.createSprite(Render.loadImage("[game]/robobug.png"), 0, 0, 1, 1)
        __cmdSprite = Render.createSprite(Render.loadImage("[game]/grdcmd.png"), 0, 0, 1, 1)

        spawnEnemies()
    }    

    static spawnEnemies() {
        __grid = Grid.new(9, 7, Type.none)

        var x = __rand.int(0, 4)
        var y = __rand.int(0, 3)
        __grid[x, y] = Type.wall
        __grid[x, 6 - y] = Type.wall
        __grid[8 - x, y] = Type.wall
        __grid[8 - x, 6 - y] = Type.wall

        var count = 0
        while(count < __level) {
            var x = __rand.int(0, __grid.width)
            var y = __rand.int(0, __grid.height)
            if(__grid[x, y] == Type.none) {
                __grid[x, y] = Type.enemy
                count = count + 1
            }
        }

        count = 0
        while(count < 1) {
            var x = __rand.int(0, __grid.width)
            var y = __rand.int(0, __grid.height)
            if(__grid[x, y] == Type.none) {
                __grid[x, y] = Type.player
                count = count + 1
            }
        }
    }

    // The update method is called once per tick.
    // Gameplay code goes here.
    static update(dt) {
        __time = __time + dt

        if(__turn == Turn.player) {
            playerTurn()        
        } else if(__turn == Turn.enemy) {
            enemyTurn()
        }
    }

    static playerTurn() {
        var player = null
        for(x in 0...__grid.width) {
            for(y in 0...__grid.height) {
                var val = __grid[x, y]
                if(val == Type.player) {
                    player = Vec2.new(x, y)
                }
            }
        }

        if(player == null) {
            __turn = Turn.dead
            return
        }

        var direction = getDirection()
        if(direction != Vec2.new(0, 0)) {
            moveDirection(player, direction)
            __turn = Turn.enemy
        }
    }

    static enemyTurn() {
        __turn = Turn.player
        var playerPos = null
        for(x in 0...__grid.width) {
            for(y in 0...__grid.height) {
                var val = __grid[x, y]
                if(val == Type.player) {
                    playerPos = Vec2.new(x, y)
                }
            }
        }

        if(playerPos == null) {
            __turn = Turn.dead
            return
        }

        var enemies = List.new()
        for(x in 0...__grid.width) {
            for(y in 0...__grid.height) {
                var enemy = __grid[x, y]
                if(enemy == Type.enemy) {
                    var pos = Vec2.new(x, y)
                    enemies.add(pos)
                }
            }
        }

        if(enemies.count == 0) {
            __level = __level + 1
            __turn = Turn.player
            spawnEnemies()
            return
        }

        for(ePos in enemies) {
            var dir = playerPos - ePos
            dir = manhattanize(dir)
            var nPos = ePos + dir
            if(__grid[nPos.x, nPos.y] != Type.enemy) {
                moveDirection(ePos, dir) 
            }
        }
    }

    static manhattanize(dir) {
        if(dir.x.abs > dir.y.abs) {
            return Vec2.new(dir.x.sign, 0)
        } else {
            return Vec2.new(0, dir.y.sign)
        }
    }

    static getDirection() {
        if(Input.getKeyOnce(Input.keyUp)) {
            return Vec2.new(0, 1)
        } else if(Input.getKeyOnce(Input.keyDown)) {
            return Vec2.new(0, -1)
        } else if(Input.getKeyOnce(Input.keyLeft)) {
            return Vec2.new(-1, 0)
        } else if(Input.getKeyOnce(Input.keyRight)) {
            return Vec2.new(1, 0)
        } else {
            return Vec2.new(0, 0)
        }
    }

    static moveDirection(position, direction) {
        var from = position
        var to = position + direction
        to.x = Math.mod(to.x, __grid.width)
        to.y = Math.mod(to.y, __grid.height)
        if(__grid[to.x, to.y] != Type.none) {
            __grid[to.x, to.y] = Type.none            
        }
        __grid.swap(from.x, from.y, to.x, to.y)
    }

    // The render method is called once per tick, right after update.
    static render() {
        Render.sprite(__clear, 0, 0, -1, 180, 0, 0x0000A8FF, 0x00000000, Render.spriteCenter)

        var pc = Data.getColor("Player Color")
        //var r = 6
        var of = 20
        var sx = ((__grid.width - 1) / 2 * of).round
        var sy = ((__grid.height - 1) / 2 * of).round 

        for(x in 0...__grid.width) {
            for(y in (__grid.height - 1)..0) {
                var val = __grid[x, y]
                if(val == Type.wall) {
                    //Render.sprite(__gridTiles, x * of - sx, y * of - sy + 6, 1.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, Render.spriteCenter)
                } else {
                    Render.sprite(__gridTiles, x * of - sx, y * of - sy, 1.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, Render.spriteCenter)
                }

                if(val == Type.player) {
                    Render.sprite(__cmdSprite, x * of - sx, y * of - sy + 3, 1.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, Render.spriteCenter)
                } else if(val == Type.enemy) {
                    Render.sprite(__bugSprite, x * of - sx, y * of - sy + 2, 1.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, Render.spriteCenter)
                }
            }
        }

        Render.setColor(0xFFFF00FF)
        Render.shapeText("GRDCMD.EXE", -29, 82, 1)

        Render.setColor(0xA8A8A8FF)
        Render.line(-35, 79, -100, 79)
        Render.line(35, 79, 100, 79)
        Render.line(100, -82, 100, 79)
        Render.line(-100, -82, -100, 79)
        Render.line(-35, -82, -100, -82)
        Render.line(35, -82, 100, -82)        

        Render.setColor(0x0407ACFF)
        Render.rect(-90, -81, 90, -75)
        

        if(__turn == Turn.dead) {
            Render.setColor(0xFF0107FF)
            Render.shapeText("TERMINATED", -30, -78, 1)     
        } else {
            Render.setColor(0xA8A8A8FF)
            //Render.setColor(0xFFFF00FF)
            Render.shapeText("LV:%(__level - 2) SC:10", -29, -78, 1)
        }
    }
}