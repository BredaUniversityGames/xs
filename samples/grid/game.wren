// Import the necessary modules
import "xs" for Render, Input, Data // The engine-level xs API
import "xs_math" for Math, Color    // Math and Color functionality
import "xs_tools" for ShapeBuilder  
import "xs_containers" for Grid     // Grid is a 
import "background" for Background  // Wobbly background - local module
import "random" for Random          // Random number generator - system module

// Just some names for the different things that can be on the grid
class Type {
    static empty       { 0 << 0 }   // This is actually a function that returns 0
    static tree_one    { 1 << 0 }   // Static means that the function is a class function
    static tree_two    { 1 << 1 }   // The << operator is a bit shift operator
    static tree_three  { 1 << 2 }   // The value of each type is a power of 2
    static grass_one   { 1 << 3 }   // This is so that they can be combined into a single value
    static grass_two   { 1 << 4 }   // This is useful for collision detection
    static grass_three { 1 << 5 }   // For example, a player can be on a tree and grass
    static road        { 1 << 6 }   // but not on two trees at the same time
    static player      { 1 << 7 }   // The player is a special type of tile
}

class Game {
    static pickOne(list) {
        return list[__random.int(0, list.count)]
    }

    static initialize() {
        __random = Random.new() // Create a new random number generator
        __width = Data.getNumber("Level Width", Data.game)  // Get the width of the level from the game.json data file.
        __height = Data.getNumber("Level Height", Data.game) // All Data variables are visible from the UI
        __background = Background.new() // All variables that start with __ are static variables
        var image = Render.loadImage("[game]/assets/monochrome-transparent_packed.png") // Load the image in a local variable
        __tileSize = 16
        var r = 49
        var c = 22
        __tiles = {            
            Type.empty: Render.createGridSprite(image, r, c, 624),
            Type.tree_one: Render.createGridSprite(image, r, c, 51),
            Type.tree_two: Render.createGridSprite(image, r, c, 52),
            Type.tree_three: Render.createGridSprite(image, r, c, 53),
            Type.grass_one: Render.createGridSprite(image, r, c, 5),
            Type.grass_two: Render.createGridSprite(image, r, c, 1),
            Type.grass_three: Render.createGridSprite(image, r, c, 6),
            Type.road: Render.createGridSprite(image, r, c, 3),
            Type.player: Render.createGridSprite(image, r, c, 76)            
        }        

        __grid = Grid.new(__width, __height, Type.empty) // Create a new grid with the width and height
        
        // Fill the grid with grass
        for(i in 0...__width) {
            for(j in 0...__height) {
                var grass = Game.pickOne([Type.grass_one, Type.grass_one, Type.grass_two, Type.grass_three])
                __grid[i, j] = grass 
            }
        }

        // Add some trees
        for(i in 0...__width) {
            for(j in 0...__height) {
                if (__random.int(0, 100) < 30) {
                    var tree = Game.pickOne([Type.tree_one, Type.tree_two, Type.tree_three])
                    __grid[i, j] = tree
                }
            }
        }

        // Add a road in the middle that also goes up and down a bit
        var j = __height / 2
        for(i in 0...__width) {
            __grid[i, j] = Type.road
            if (__random.int(0, 100) < 50) {
                j = j + __random.int(-1, 2)
            }
            if (j < 0) {
                j = 0
            }
            if (j >= __height) {
                j = __height - 1
            }
        }

        // Add the player
        __playerX = 0
        __playerY = __height / 2
    }

    static update(dt) {
        __background.update(dt)

        // Move the player
        var dx = 0
        var dy = 0
        if (Input.getKeyOnce(Input.keyA)) {
            dx = -1
        }
        if (Input.getKeyOnce(Input.keyD)) {
            dx = 1
        }
        if (Input.getKeyOnce(Input.keyS)) {
            dy = -1
        }
        if (Input.getKeyOnce(Input.keyW)) {
            dy = 1
        }

        if (dx != 0 || dy != 0) {
            var nx = __playerX + dx
            var ny = __playerY + dy
            if (__grid.valid(nx, ny)) {
                if (__grid[nx, ny] != Type.tree_one && __grid[nx, ny] != Type.tree_two && __grid[nx, ny] != Type.tree_three) {
                    __playerX = nx
                    __playerY = ny
                }
            }
        }
    }

    static render() {
        __background.render()
        var s = __tileSize  
        var sx = (__width - 1) * -s / 2
        var sy = (__height - 1)  * -s / 2        
        for (x in 0...__width) {
            for (y in 0...__height) {
                if(x == __playerX && y == __playerY) {
                    var tile = __tiles[Type.player]
                    Render.sprite(tile, sx + x * s, sy + y * s, 0.0, 1.0, 0.0, Data.getColor("Player Color"), 0x0, Render.spriteCenter)
                } else {
                    var t = __grid[x, y]
                    var px = sx + x * s
                    var py = sy + y * s
                    var tile = __tiles[t]            
                    Render.sprite(tile, px, py, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
                }
            }
        }
    }
}