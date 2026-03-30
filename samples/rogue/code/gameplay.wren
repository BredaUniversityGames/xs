import "xs/core" for Data, Input, Render
import "xs/math"for Math, Bits, Vec2, Color
import "xs/ec"for Entity, Component
import "xs/components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite
import "xs/containers" for Grid, SparseGrid, Queue
import "xs/tools" for Tools
import "random" for Random
import "types" for Type
import "directions" for Directions

class MoveAnimation is Component {
    construct new() {
        super()
        _duration = 0.2 // TODO: Make this configurable from the game data
        _time = 0.0
        _from = Vec2.new(0, 0)
        _to = Vec2.new(0, 0)
        _state = MoveAnimation.idle
    }

    initialize() {
        _transform = owner.get(Transform)
        _tile = owner.get(Tile)
    }    

    /// Update the animation and return the current position of the animation
    update(dt : Num) {        
        if(_state == MoveAnimation.idle) return
        // System.print("movin")        
        if(_state == MoveAnimation.moving) {
            _time = _time + dt
            var t : Num = _time / _duration            
            if(t > 1.0) {
                t = 1.0
                _state = MoveAnimation.idle
            }
            t = t.pow(6.0)
            var pos = Vec2.lerp(_from, _to, t)
            _transform.position = pos        
        } else if(_state == MoveAnimation.attacking) {
            _time = _time + dt
            var t: Num = _time / (_duration * 0.5)
            t = t % 1.0
            t = t.pow(6.0)
            if(_time < _duration * 0.5) {
                var pos = Vec2.lerp(_from, _to, t)
                _transform.position = pos
            } else if(_time < _duration) {
                var pos = Vec2.lerp(_to, _from, t)
                _transform.position = pos
            } else {
                _transform.position = _from
                _state = MoveAnimation.idle
                _time = 0.0
            }
        }
    }

    move(direction : Num) {
        var d = Directions[direction]
        _from = Level.calculatePos(_tile)
        _to = Level.calculatePos(_tile.x + d.x, _tile.y + d.y)
        _time = 0.0
        _state = MoveAnimation.moving
    }

    attack(direction : Num) {
        var d = Directions[direction]
        _from = Level.calculatePos(_tile)
        var to: Vec2 = Level.calculatePos(_tile.x + d.x, _tile.y + d.y)
        _to = Vec2.lerp(_from, to, 0.5)
        _time = 0.0
        _state = MoveAnimation.attacking
    }

    done { _time >= _duration }

    static idle { 0 }
    static moving { 1 }
    static attacking { 2 }
}

/// Contains the level data and the logic to manipulate it
/// It's completely static and should be used as a singleton
class Level {    
    
    /// Initialize the level with the data from the game
    /// Must be called before using the Level class
    static initialize() {
        __tileSize = Data.getNumber("Tile Size", Data.game)
        __width = Data.getNumber("Level Width", Data.game)
        __height = Data.getNumber("Level Height", Data.game)
        
        __grid = Grid.new(__width, __height, Type.empty)
        __rendering = Grid.new(__width, __height, null)
        __rooms = []
    }

    /// Calculate the position of a tile in the level
    static calculatePos(tile : Tile) -> Vec2 {
        return calculatePos(tile.x, tile.y)
    }

    /// Calculate the position of a tile in the level
    static calculatePos(tx : Num, ty : Num) -> Vec2 {
        var sx = (__width - 1) * -__tileSize / 2.0
        var sy = (__height - 1)  * -__tileSize / 2.0
        var px = sx + tx * __tileSize
        var py = sy + ty * __tileSize
        return Vec2.new(px, py)        
    }

    /// Calculate the tile position of a given position in the level
    static calculateTile(pos : Vec2) -> Vec2 {
        var sx = (__width - 1.0) * -__tileSize / 2.0
        var sy = (__height - 1.0)  * -__tileSize / 2.0
        var tx = (pos.x - sx) / __tileSize
        var ty = (pos.y - sy) / __tileSize
        return Vec2.new(tx.round, ty.round)
    }

    /// Get the tile at a given position (used for rendering)
    static tileSize -> Num { __tileSize }
    
    /// Get the width of the level (in tiles)
    static width -> Num { __width }

    /// Get the height of the level (in tiles)
    static height -> Num { __height }

    /// The random number generator used in the level
    static random -> Random { __random }

    /// The grid that contains the logical representation of the level (used for gameplay)
    static gameplay -> Grid { __grid }

    /// The grid that contains the rendering representation of the level (used for rendering)
    static rendering -> Grid { __rendering }

    /// The list of rooms in the level (used for gameplay and rendering)
    static rooms -> List { __rooms }
}

// A component that represents a tile in the level
// It is used to store the position of the tile in the level
// but also to store all the tiles in the level as a static variable
class Tile is Component {

    /// Must be called from the game before using the Tile class
    static initialize() {
        __tiles = SparseGrid.new()
    }

    /// Create a new tile at a given position
    construct new(x : Num, y : Num) {
        super()
        _x = x
        _y = y
        System.print("Creating tile at position [%(x),%(y)]")
        __tiles[x, y] = this
    }

    // Cache the transform component of the tile for faster access
    initialize() {
        //_transform = owner.get(Transform)

    }

    // Update the tile position in the level based on the transform component
    update(dt : Num) {
        // _transform.position = Level.calculatePos(this)
    }

    /// Get the tile at a given position
    static get(x : Num, y : Num) -> Tile {
        if(__tiles.has(x, y)) return __tiles[x, y]
        return null
    }

    /// Move the tile to a new position with a given offset
    move(dx : Num, dy : Num) {  
        __tiles.remove(_x, _y)
        _x = _x + dx
        _y = _y + dy
        __tiles[_x, _y] = this
    }

    /// Remove the tile from the level (gets called when the entity is deleted)
    finalize() {
        // Check if the tile has not been replaced already
        if(__tiles[_x, _y] == this) {
            __tiles.remove(_x, _y)
        }
    }
    
    /// Get the x position of the tile
    x -> Num { _x }

    /// Get the y position of the tile
    y -> Num { _y }
}

class Stats is Component {
    construct new(health : Num, damage : Num, armor : Num, drop : Num) {
        _health = health    // Health points
        _damage = damage    // Damage points
        _armor = armor      // Armor points
        _drop = drop        // Drop chance
    }

    /// Clone the stats - used to create a copy of the stats and modify them
    /// without changing the original. Useful for creating new entities with
    /// similar stats
    clone() -> Stats { Stats.new(_health, _damage, _armor, _drop) }

    add(other : Stats) {
        _health = _health + other.health
        _damage = _damage + other.damage
        _armor = _armor + other.armor
        _drop = _drop + other.drop
    }

    health -> Num { _health }
    damage -> Num { _damage }
    armor -> Num { _armor }
    drop -> Num { _drop }

    health=(v : Num) { _health = v }
    damage=(v : Num) { _damage = v }
    armor=(v : Num) { _armor = v }
    drop=(v : Num) { _drop = v }
}

/// A base class for all characters in the game
/// Used by the hero and the monsters
class Character is Component {
    /// Create a new character with a given type of attackable entities
    construct new(attackable) {
        _attackable = attackable
        _direction = Directions.downIdx
    }

    /// Initialize the character by caching the stats and the tile
    initialize() {
        _stats = owner.get(Stats)
        _tile = owner.get(Tile)
        _mover = owner.get(MoveAnimation)
    }

    /// Update the character - just debug rendering for now
    update(dt) {
        if(Data.getBool("Debug.Draw", Data.game)) {
            var pos = Level.calculatePos(_tile)
            Render.dbgColor(0xFFFFFFFF)
            Render.dbgText("%(owner.name)", pos.x - 7, pos.y + 7, 1)
        }
    }

    // Implement turn logic here one and return true when done
    turn() { true }  

    /// Check if the tile in the direction has a given type flag
    checkTile(dir, type) {
        var d = Directions[dir]
        var x = _tile.x + d.x
        var y = _tile.y + d.y
        var flag = Level.gameplay[x, y]
        var t = Tile.get(x, y)
        if(t != null) flag = flag | t.owner.tag // |
        return Bits.checkBitFlagOverlap(type, flag)
    }

    /// Move the tile in the direction
    moveTile(dir) {
        var d = Directions[dir]
        _mover.move(dir)
        _tile.move(d.x, d.y)        
    }

    /// Attack the tile in the direction
    attackTile(dir) {
        System.print("Attacking from position [%(_tile.x),%(_tile.y)] in direction [%(dir)]")
        var d = Directions[dir]
        var x = _tile.x + d.x
        var y = _tile.y + d.y
        var t = Tile.get(x, y)
        if(t != null) {
            if(Bits.checkBitFlag(_attackable, t.owner.tag)) {
                var stats = t.owner.get(Stats)
                // TODO: Calculate hit chance based on the target's armor (max 80% hit chance) 
                // TODO: Don't calculate damage if the attack misses
                // TODO: Expose the hit chance and the damage in the UI (maybe as a message)
                var hitChance = 0.8 - stats.armor * 0.1                 
                var hit = Tools.random.float(0.0, 1.0) < hitChance
                if(hit) {
                    stats.health = stats.health - _stats.damage
                    Gameplay.message =  "%(owner.name) deals %(_stats.damage) damage to %(t.owner.name)"
                } else {
                    Gameplay.message = "%(owner.name) misses %(t.owner.name)"
                }

                if(stats.health <= 0) {
                    Gameplay.message = "%(owner.name) kills %(t.owner.name)"
                    t.owner.delete()
                    if(Tools.random.float(0.0, 1.0) < stats.drop) Create.item(x, y)                    
                }
                _mover.attack(dir)
            } else if(Bits.checkBitFlag(Type.item, t.owner.tag)) {
                Gameplay.message = "%(owner.name) picks up %(t.owner.name)"
                _stats.add(t.owner.get(Stats))
                t.owner.delete()
                moveTile(dir)  
            }
        }
    }

    /// Get the tile of the character
    tile { _tile }
}

/// A class that represents the hero of the game
/// The hero is also a singleton, so there is only one hero in the game
class Hero is Character {    
    /// Create a new hero component
    construct new() {
        super(Type.enemy)
        _buttons = [Input.gamepadDPadUp,
                    Input.gamepadDPadRight,
                    Input.gamepadDPadDown,
                    Input.gamepadDPadLeft ]
        _keys = [   Input.keyUp,
                    Input.keyRight,
                    Input.keyDown,
                    Input.keyLeft]
        __hero = this
    }

    /// Finalize the hero singleton by setting it to null
    finalize() {
        __hero = null
        Gameplay.message = "The hero has fallen"
    }

    /// Player turn logic
    turn() {     
        var dir = getDirection()
        if(dir >= 0) {
            _direction = Directions[dir]
            if(checkTile(dir, Type.enemy | Type.item)) { // |
                attackTile(dir)
            } else if(!checkTile(dir, Type.blocking)) {
                moveTile(dir)
            }
            return true
        }    
        return false
    }

    /// Get the direction of the player input
    getDirection() {
        for(dir in 0...4) {
            if(Input.getButtonOnce(_buttons[dir]) || Input.getKeyOnce(_keys[dir])) {
                return dir
            }
        }
        return -1
    }

    /// Player singleton turn
    static turn() {
        if(__hero) return __hero.turn()
    }

    /// Get the hero singleton
    static hero { __hero }
 }


/// A class that represents the monsters in the game
/// The monsters are controlled by the computer and the class
/// contains the logic play a turn for all the monsters
class Monster is Character {
    /// Create a new monster component
    construct new() {
        super(Type.player)         
    }

    /// Single monster turn logic
    turn() {
        if(!enabled) return

        var dir = getDirection()                                                         
        if(dir >= 0) {
            _direction = Directions[dir]
            if(checkTile(dir, Type.player)) {
                attackTile(dir)
            } else if(!checkTile(dir, Type.blocking)) {
                moveTile(dir)
            }
        } 
    }

    /// Get the direction of the monster
    getDirection() {
        if(__fill) {
            if(__fill.has(tile.x, tile.y)) {
                return __fill[tile.x, tile.y]
            }
        }
        return -1
    }

    /// Computer turn logic for all the monsters
    static turn() {
        var enemies = Entity.withTagOverlap(Type.enemy)        
        for(e in enemies) {
            floodFill()
            var s = e.get(Monster)
            s.turn()                        
        }
        return true
    }

    /// An algorithm to fill the level with the directions to the hero    
    static floodFill() {
        if(Hero.hero) {
            var hero = Hero.hero.owner.get(Tile)
            var open = Queue.new()
            open.push(Vec2.new(hero.x, hero.y))
            __fill = SparseGrid.new()
            __fill[hero.x, hero.y] = Directions.noneIdx
            var count = 50
            while(!open.empty() && count > 0) {
                var next = open.pop()
                for(i in 0...4) {
                    var nghb = next + Directions[i]
                    if(Level.gameplay.valid(nghb.x, nghb.y) && !__fill.has(nghb.x, nghb.y)) {
                        var flags = Gameplay.getFlags(nghb.x, nghb.y)
                        if(!Bits.checkBitFlagOverlap(flags, Type.monsterBlock)) {
                            __fill[nghb.x, nghb.y] = (i + 2) % 4 // Opposite direction 
                            open.push(nghb)
                        }
                    }                     
                }
                count = count - 1
            }   
        }
    }

    /// Debug render the flood fill algorithm
    static debugRender() { 
        Render.dbgColor(0xFF0000FF)    
        if(__fill != null) { 
            for (x in 0...Level.width) {
                for (y in 0...Level.height) {
                    if(__fill.has(x, y)) { 
                        var dr = Directions[__fill[x, y]]
                        var fr = Level.calculatePos(x, y)
                        var to = Level.calculatePos(x + dr.x, y + dr.y)
                        Render.dbgLine(fr.x, fr.y, to.x, to.y)
                    }
                }
            }
        }
    }
 }

 class UI {
    construct new() {
        _font = Render.loadFont("[game]/assets/FutilePro.ttf", 14)
        var icons : Num = Render.loadImage("[game]/assets/monochrome.png")        
        _heart = Render.createGridSprite(icons, 49, 22, 532)
        _armor = Render.createGridSprite(icons, 49, 22, 236)
        _sword = Render.createGridSprite(icons, 49, 22, 326)
    }

    render() {
        var hero = Hero.hero
        var stats = hero.owner.get(Stats)
        // var message = "Health: %(stats.health)  Damage: %(stats.damage)  Armor: %(stats.armor)"

        // Health
        Render.sprite(_heart, -160, 70, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
        Render.text(_font, "%(stats.health)", -160, 50, 1.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)

        // Armor
        Render.sprite(_armor, -160, 20, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
        Render.text(_font, "%(stats.armor)", -160, 0, 1.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)

        // Damage
        Render.sprite(_sword, -160, -30, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
        Render.text(_font, "%(stats.damage)", -160, -50, 1.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
    }
 }

 class Visibility {
    static hidden { 0 }
    static visited { 1 }
    static visible { 2 }
 }

/// Combines level and character logic to create the gameplay
class Gameplay {
    static playerTurn   { 1 }
    static computerTurn { 2 }

    static initialize() {
        __state = playerTurn
        __font = Render.loadFont("[game]/assets/FutilePro.ttf", 14)

        var preview = Render.loadImage("[game]/assets/tileset.png")
        var r = 16
        var c = 16
        __tiles = {
            Type.empty: Render.createGridSprite(preview, r, c, 0),
            Type.floor: Render.createGridSprite(preview, r, c, 66),
            Type.wall: Render.createGridSprite(preview, r, c, 18),
            Type.player: Render.createGridSprite(preview, r, c, 128),
            Type.mage: Render.createGridSprite(preview, r, c, 164),
            Type.skeleton: Render.createGridSprite(preview, r, c, 148),
            Type.ghost: Render.createGridSprite(preview, r, c, 132),
            Type.knight: Render.createGridSprite(preview, r, c, 180),
            Type.crusader: Render.createGridSprite(preview, r, c, 196),
            Type.slime: Render.createGridSprite(preview, r, c, 212)        
        }

        __message = "A hero is born"
        __timer = Data.getNumber("Message Time", Data.game)    
        __visibility = Grid.new(Level.width, Level.height, Visibility.hidden)
        __ui = UI.new()
        __room = null
    }    

    static update(dt) {
        __timer = __timer - dt 
        if(__timer <= 0) {
            __message = ""
        }

        if(__state == Gameplay.playerTurn) {
            if(Hero.turn()) {
                __state = Gameplay.computerTurn
            }
        } else if(__state == Gameplay.computerTurn) {
            if(__timer <= 0) {
                if(Monster.turn()) {
                    __state = Gameplay.playerTurn
                }            
            }
        }

    
        var room = getCurrentRoom()
        if(room != null) __room = room 
        enableEntitiesInCurrentRoom()
    }

    static getCurrentRoom() {
        var hero: Entity = Hero.hero.owner
        var tile: Tile = hero.get(Tile)

        for(r: Rect in Level.rooms) {
            if(r.contains(tile)) {
                return r
            }
        }
        return null
    }

    static enableEntitiesInCurrentRoom() {
        var hero: Entity = Hero.hero.owner
        var tile: Tile = hero.get(Tile)

        if(__room != null) {
            var entities = Entity.withTagOverlap(Type.enemy)
            for(e in entities) {
                var t = e.get(Tile)
                if(t != null && __room.contains(t)) {
                    e.enabled = true
                }
            }
        }
    }

    static getFlags(x, y) {
        if(Level.gameplay.valid(x, y)) {
            var flag = Level.gameplay[x, y]
            var t = Tile.get(x, y)
            if(t != null) flag = flag | t.owner.tag // |
            return flag
        } else {
            return
        }
    }

    /// Render the level and the UI
    static renderGeneration() {
        var s = Level.tileSize  
        var sx = (Level.width - 1) * -s / 2
        var sy = (Level.height - 1)  * -s / 2        
        for (x in 0...Level.width) {
            for (y in 0...Level.height) {
                var px = sx + x * s
                var py = sy + y * s
                
                var tl = Tile.get(x, y)
                if(tl != null) {
                    var tag = tl.owner.tag
                    Render.sprite(__tiles[tag], px, py, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
                    continue
                }

                var t = Level.gameplay[x, y]                
                if(t == Type.empty) continue
                var color : Num = 0xFFFFFFFF
                var sprite = __tiles[t]
                if(sprite != null) {
                    Render.sprite(sprite, px, py, 0.0, 1.0, 0.0, color, 0x0, Render.spriteCenter)
                }
            }
        }
    }  

    static render() {        
        System.print("Rendering gameplay")
        if(Hero.hero) {
            var hero: Entity = Hero.hero.owner
            var tile: Tile = hero.get(Tile)

            // Reset visibility of all tiles to visited if they are currently visible
            for(x in 0...Level.width) {
                for(y in 0...Level.height) {
                    if(__visibility[x, y] == Visibility.visible) {
                        __visibility[x, y] = Visibility.visited
                    }
                }
            }            
        
            // Set visibility of the tiles in the current room to visible
            if(__room != null) {
                var level: Rect = Rect.new(0, 0, Level.width, Level.height)
                var extended = Rect.new(__room.from - Vec2.new(2, 2), __room.to + Vec2.new(2, 2))
                var room: Rect = extended.interection(level)
                
                for(x in room.from.x...room.to.x) {
                    for(y in room.from.y...room.to.y) {
                        var px = Level.calculatePos(x, y).x
                        var py = Level.calculatePos(x, y).y
                        var r = Level.rendering[x, y]
                        if(r != null) {
                            __visibility[x, y] = Visibility.visible                            
                        }
                    }
                }                
            }

            // Make a small area around the hero visible even if they are not in a room
            var radius = Data.getNumber("Render Radius", Data.game)
            for (x in tile.x - radius...tile.x + radius) {
                for (y in tile.y - radius...tile.y + radius) {
                    if(Level.gameplay.valid(x, y)) {
                        var px = Level.calculatePos(x, y).x
                        var py = Level.calculatePos(x, y).y
                        var r = Level.rendering[x, y]
                        if(r != null) {
                            __visibility[x, y] = Visibility.visible
                        }
                    }
                }
            }

            // Render the level based on the visibility of the tiles
            for(x in 0...Level.width) {
                for(y in 0...Level.height) {
                    var r = Level.rendering[x, y]
                    if(r == null) continue
                    var color : Num = 0xFFFFFF00
                    if(__visibility[x, y] == Visibility.visited) color = 0xFFFFFF50
                    if(__visibility[x, y] == Visibility.visible) color = 0xFFFFFFFF
                    var px = Level.calculatePos(x, y).x
                    var py = Level.calculatePos(x, y).y
                    var t = Level.gameplay[x, y]
                    Render.sprite(r, px, py, 0.0, 1.0, 0.0, color, 0x0, Render.spriteCenter)                        
                }
            }

            // Render the tiles around the hero
            /* 
            var radius = Data.getNumber("Render Radius", Data.game)
            for (x in tile.x - radius...tile.x + radius) {
                for (y in tile.y - radius...tile.y + radius) {
                    if(!Level.gameplay.valid(x, y)) continue
                    var px = Level.calculatePos(x, y).x
                    var py = Level.calculatePos(x, y).y
                    var r = Level.rendering[x, y]
                    if(r != null) {
                        Render.sprite(r, px, py, 0.0, 1.0, 0.0, 0xFFFFFF90, 0x0, Render.spriteCenter)
                    }
                }
            }
            */

            __ui.render()
        }        
    }

    static message=(v) {
        __message = v
        __timer = Data.getNumber("Message Time", Data.game)
    }    
 }

import "create" for Create
import "generators" for Rect