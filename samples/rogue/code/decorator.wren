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
        // TODO: Fill in with actual sprite indices from your tileset
        var wallMap = {
            0: [13, 14, 15],    // No neighbors - isolated wall
            1: [18, 20, 22, 24, 26, 28],      // N
            2: [254],      // E
            3: [254],      // NE corner
            4: [254],      // S
            5: [254],      // NS vertical
            6: [2, 6, 10],      // SE corner
            7: [254],      // NES T-junction
            8: [254],      // W
            9: [254],      // NW corner
            10: [11],    // EW horizontal
            11: [254],    // NEW T-junction
            12: [4, 8 , 12],    // SW corner
            13: [254],    // NSW T-junction
            14: [254],    // ESW T-junction
            15: [254]     // NESW cross/full
        }
        
        var floorMap = {
            0: [255],     // No neighbors - isolated floor
            1: [255],     // N
            2: [255],     // E
            3: [255],     // NE
            4: [255],     // S
            5: [255],     // NS
            6: [255],     // SE
            7: [255],     // NES
            8: [255],     // W
            9: [255],     // NW
            10: [255],    // EW
            11: [255],    // NEW
            12: [255],    // SW
            13: [255],    // NSW
            14: [255],    // ESW
            15: [255]     // NESW
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
                    System.print("Tile [%(x),%(y)] type=%(tileType) mask=%(mask) sprite=%(spriteIndex)")
                }
            }
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