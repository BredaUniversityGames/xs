import "xs" for Input, Render, Data

class Vec2 {
    construct new(x, y) {
        _x = x
        _y = y
    }

    x=(value) { _x = value }
    x { _x }
    y=(value) { _y = value }
    y { _y }
}

class HexTile {
    construct new(q, r) {        
        _q = q
        _r = r
    }

    construct new(x, y, z) {
        _q = x
        _r = z
    }

    q=(value) { _q = value }
    q { _q }
    r=(value) { _r = value }
    r { _r }

    x=(value) { _q = value }
    x { _q }
    z=(value) { _r = value }
    z { _r }
    y { -x-z }

    static distance(a, b) {
        return ((a.q - b.q).abs  + (a.q + a.r - b.q - b.r).abs + (a.r - b.r).abs) / 2.0
    }
}

class HexGrid {

    construct new(size) {
        _size = size
        _grid = Map.new()
    }

    getPosiion(tile) {
        var v = Vec2.new(0, 0)
        v.x = _size * (3.0 / 2.0 * tile.x)
	    v.y = -_size * (3.sqrt * (tile.y + tile.x / 2.0))
        return v
    }

    getTile(position) {
        var q = position.x * 2.0 / 3.0 / _size
	    var r = (-position.x / 3.0 + 3.sqrt / 3.0 * position.y) / _size

	    var cx = q
	    var cz = r
	    var cy = -cx-cz

	    var rx = cx.round
	    var ry = cy.round
	    var rz = cz.round

	    var x_diff = (rx - cx).abs
	    var y_diff = (ry - cy).abs
	    var z_diff = (rz - cz).abs

	    if ((x_diff > y_diff) && (x_diff > z_diff)) {
		    rx = -ry - rz
        } else if (y_diff > z_diff) {
		    ry = -rx - rz
        } else {
		    rz = -rx - ry
        }

	    return Vec2.new(rx, ry)
    }
}

class Game {

    static init() {
        __hexes = HexGrid.new(16)
        __pos = Vec2.new(0.0, 0.0)
    }    

    static config() {
        Data.setString("Title", "Rogue", Data.system)
        Data.setNumber("Width", 280, Data.system)
        Data.setNumber("Height", 280, Data.system)
        Data.setNumber("Multiplier", 2, Data.system)
    }
    static render() {}
    
    static update(dt) {        
        __pos.x = __pos.x + Input.getAxis(0)
        __pos.y = __pos.y - Input.getAxis(1)

        Render.setColor(1, 0, 1)
        Render.circle(__pos.x, __pos.y, 5, 6)
        Render.circle(__pos.x, __pos.y, 4, 6)
        Render.circle(__pos.x, __pos.y, 3, 6)

        var origin = HexTile.new(0, 0)
        var playerTile = __hexes.getTile(__pos)
        var playerPos = __hexes.getPosiion(playerTile)
        Render.setColor(1, 0, 1)
        Render.circle(playerPos.x, playerPos.y, 7, 6)
        
        Render.setColor(1, 1, 1)
        var bl = -256 + 16
        for (x in -7..7) {
            for(y in -7..7) {                
                var tile = HexTile.new(x, y)                
                if(HexTile.distance(tile, origin) < 5) {
                    var pos = __hexes.getPosiion(tile)
                    Render.circle(pos.x, pos.y, 14, 6)
                }
            }
        } 
    }    
}
