import "xs/core" for Data, Input, Render
import "xs/math" for Vec2, Math
import "xs/containers" for Queue
import "xs/components" for GridSprite
import "gameplay" for Level, Tile
import "types" for Type
import "directions" for Directions
import "random" for Random

class Decorator {   
    static decorate() {
        //get some data that we will use later
        var shortBrake = Data.getNumber("Short Brake")
        var longBrake = Data.getNumber("Long Brake")
        var width = Level.width
        var height = Level.height
        var random = Random.new()

        // Load all the tiles that are floor for rendering 
        var tilest = Render.loadImage("[game]/assets/tileset.png")        
        var rows : Num = 16
        var cols : Num = 16
        var tiles : List = []
        for(i in 0...256) {
            var gs = Render.createGridSprite(tilest, rows, cols, i)
            tiles.add(gs)
        }

        // Manual mapping: mask value -> sprite indices (can be multiple options)
        var bottom = [3, 7, 11, 34, 36, 38, 40, 42, 44]
        var middle = [18, 20, 22, 24, 26, 28]
        var wallMap = {
            0: [13, 14, 15],    // No neighbors - isolated wall
            1: middle,      // N
            2: middle,      // E
            3: bottom,      // NE corner
            4: middle,      // S
            5: middle,      // NS vertical
            6: [2, 6, 10],      // SE corner
            7: middle,      // NES T-junction
            8: middle,      // W
            9: bottom,      // NW corner
            10: bottom,    // EW horizontal
            11: bottom,    // NEW T-junction
            12: [4, 8 , 12],    // SW corner
            13: middle,    // NSW T-junction
            14: middle,    // ESW T-junction
            15: middle     // NESW cross/full
        }
        
        var rightOfWall = [17, 21, 25, 33, 37, 41]
        var floor = [48, 64]
        var floorMap = {
            0: [255],     // No neighbors - isolated floor
            1: floor,     // N
            2: floor,     // E
            3: floor,     // NE
            4: floor,     // S
            5: floor,     // NS
            6: floor,     // SE
            7: floor,     // NES
            8: rightOfWall,     // W
            9: rightOfWall,     // NW
            10: floor,    // EW
            11: floor,    // NEW
            12: [1, 5, 9],  // SW
            13: rightOfWall,    // NSW
            14: floor,    // ESW
            15: floor     // NESW
        }

        // Calculate autotile masks for each tile
        for (x in 0...width) {
            for (y in 0...height) {
                var tileType = Level.gameplay[x, y]
                
                // Only autotile walls and floors
                if (tileType == Type.wall || tileType == Type.floor) {
                    var mask = calculateMask(x, y, tileType, width, height)
                    
                    // Get possible sprite indices for this mask
                    var options = (tileType == Type.wall) ? wallMap[mask] : floorMap[mask]
                    
                    // Pick randomly if multiple options exist
                    var spriteIndex = (options.count > 1) ? options[random.int(0, options.count)] : options[0]
                    var sprite = tiles[spriteIndex]
                    
                    // Store sprite in rendering grid
                    Level.rendering[x, y] = sprite
                    // System.print("Tile [%(x),%(y)] type=%(tileType) mask=%(mask) sprite=%(spriteIndex)")
                } 
            }
            Fiber.yield(0.0)
        }

        return 0.0
    }
    
    // Calculate 4-directional bitmask for autotiling
    // Returns 0-15 based on which neighbors match the tile type
    // Coordinate system: Y increases upward (cartesian coords)
    //   North = up = y+1
    //   South = down = y-1
    static calculateMask(x, y, tileType, width, height) {
        var mask = 0
        
        // North (bit 0 = 1) - checks UP (y+1)
        if (y < height - 1 && Level.gameplay[x, y+1] == tileType) {
            mask = mask | 1
        }
        
        // East (bit 1 = 2) - checks RIGHT (x+1)
        if (x < width - 1 && Level.gameplay[x+1, y] == tileType) {
            mask = mask | 2
        }
        
        // South (bit 2 = 4) - checks DOWN (y-1)
        if (y > 0 && Level.gameplay[x, y-1] == tileType) {
            mask = mask | 4
        }
        
        // West (bit 3 = 8) - checks LEFT (x-1)
        if (x > 0 && Level.gameplay[x-1, y] == tileType) {
            mask = mask | 8
        }
        
        return mask
    }
}