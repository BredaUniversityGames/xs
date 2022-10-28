import "xs" for Data, Input, Render // These are the parts of xs we will be using
import "xs_math" for Math, Vec2
import "random" for Random // random is part of the Wren library

import "grid" for Grid

/// Representation of an object in the grid.
class TileObject {
    construct new(sprite, x, y) {
        _sprite = sprite
        setCurrentCell(x, -200 + 25*y - 25*x)
        setTargetCell(x,y)
        _removed = false
    }

    setCurrentCell(x, y) {
        _currentCellX = x
        _currentCellY = y
        _screenPosition = Game.gridToScreen(x,y)
    }

    setTargetCell(x, y) {
        _targetCellX = x
        _targetCellY = y
        _targetScreenPosition = Game.gridToScreen(x,y)
    }

    remove() {
        _removed = true
    }

    isAtCell(x,y) {
        return _currentCellX == x && _currentCellY == y
    }

    isAnimating { !_removed && _screenPosition != _targetScreenPosition }

    isRemoved { _removed }

    update(dt) {
        // move to the target position
        if (isAnimating) {

            var diff = (_targetScreenPosition - _screenPosition)
            var dist = diff.magnitude

            var speed = 150 + 3 * dist

            var frameDist = dt * speed
            if (dist <= frameDist) {
                _screenPosition = _targetScreenPosition
                _currentCellX = _targetCellX
                _currentCellY = _targetCellY
            } else {
                _screenPosition = _screenPosition + diff / dist * frameDist
            }
        }
    }

    render() {
        if (!_removed) {
            Render.sprite(_sprite, _screenPosition.x, _screenPosition.y)
        }
    }
}

class TileObjectManager {
    
    /// Creates a new TileObjectManager 
    /// and creates a TileObject per value that's currently in the static Game grid.
    construct new() {

        _tileObjects = []
        for (y in 0...Game.grid.height) {
            for (x in 0...Game.grid.width) {
                var val = Game.grid.getValue(x,y)
                if (val != Game.grid.emptyValue) {
                    addTileObject(x, y, val)
                }
            }
        }
    }

    /// Adds a new TileObject with the given value and associated to the given grid cell.
    addTileObject(x, y, value) {
        _tileObjects.add(TileObject.new(Game.getSprite("cell%(value)"), x, y))
    }

    /// Finds and returns the TileObject that is associated to a given grid cell.
    /// The object may not be exactly at that position on the screen (yet), 
    /// but it is associated to the grid cell in terms of game logic.
    /// This method returns null if no such object exists (i.e. if the cell is empty in the game logic).
    findObjectAtCell(x,y) {
        // find the correct tile object
        // TODO: search more efficiently?
        for (tileObject in _tileObjects) {
            if (tileObject.isAtCell(x,y)) {
                return tileObject
            }
        }
        return null
    }

    /// Called when a grid element has moved to another cell.
    /// This method will animate the corresponding tile object to a new position.
    onElementMoved(x, y, newX, newY) {

        // find the correct tile object
        var tileObject = findObjectAtCell(x,y)

        if (tileObject != null) {
            // give the object a new target position
            tileObject.setTargetCell(newX, newY)
            Game.setPlayerCanMove(false)
        }
    }

    /// Called when a grid element has been removed.
    /// This method will let the corresponding tile object disappear.
    onElementRemoved(x, y) {
        
        // find the correct tile object
        var tileObject = findObjectAtCell(x,y)

        if (tileObject != null) {
            // mark the object for removal
            tileObject.remove()
            Game.setPlayerCanMove(false)
        }
    }

    /// Executes one frame of the game loop for all TileObjects that this TileObjectManager manages.
    /// This includes smoothly moving each TileObject towards its target position, 
    /// and removing each TileObject that needs to be removed.
    update(dt) {

        var somethingIsAnimating = false

        for (tileObject in _tileObjects) {
            tileObject.update(dt)
            if (tileObject.isAnimating) {
                somethingIsAnimating = true
            }
        }
        
        Game.setPlayerCanMove(!somethingIsAnimating)

        // remove objects that need to be removed
        var i = 0
        while (i < _tileObjects.count) {
            if (_tileObjects[i].isRemoved) {
                _tileObjects.removeAt(i)
            } else {
                i = i + 1
            }
        }
    }

    /// Renders all TileObjects that this TileObjectManager manages.
    render() {
        for (tileObject in _tileObjects) {
            tileObject.render()
        }
    }
}

/// Entry point of the game.
class Game {
    
    /// Configures basic game settings (such as window resolution) before the game starts.
    static config() {       
        // Configure the window in xs
        Data.setString("Title", "Grid Game", Data.system)
        Data.setNumber("Width", 640, Data.system)
        Data.setNumber("Height", 360, Data.system)
        Data.setNumber("Multiplier", 2, Data.system)
    }

    /// Loads an image with the given name, creates a sprite for it, and stores it in the global spritemap using the name as a key.
    static loadSprite(name) {
        var image = Render.loadImage("[games]/grid/images/%(name).png")
        __sprites[name] = Render.createSprite(image, 0, 0, 1, 1)
    }

    /// Returns the sprite with the given name (assuming it has been pre-loaded in Game.init()).
    static getSprite(name) {
        return __sprites[name]
    }

    static grid { __grid }

    static setPlayerCanMove(value) {
        __playerCanMove = value
    }

    static init() {

        __gameTime = 0 
        
        // ---
        // --- prepare the grid
        // ---
        
        // fill a grid with random values
        __grid = Grid.new(8, 8, 1, 6)
        __grid.fillRandomly()

        // make sure there are no sets immediately when the game starts
        var maxChainLength = 2
        var changed = __grid.replaceChains(maxChainLength+1, __grid.emptyValue)
        while (changed) {
            __grid.collapse()
            __grid.fillRandomly()
            changed = __grid.replaceChains(maxChainLength+1, __grid.emptyValue)
        }

        // ---
        // --- prepare variables for user input
        // ---

        __selectionX = (__grid.width / 2).floor - 1
        __selectionY = (__grid.height / 2).floor - 1

        __playerCanMove = false
        __moveStarted = false
        __moveDirections = [true, true, true, true]

        // ---
        // --- load sprites
        // ---
        
        __sprites = {}

        loadSprite("background")

        loadSprite("cell0a")
        loadSprite("cell0b")
        for (i in __grid.minValue..__grid.maxValue) loadSprite("cell%(i)")

        loadSprite("selector")
        loadSprite("arrow-left")
        loadSprite("arrow-right")
        loadSprite("arrow-up")
        loadSprite("arrow-down")

        // ---
        // --- create visual objects for each filled grid cell
        // ---

        __cellSize = 32
        __gridOffset = Vec2.new(-__grid.width/2 * __cellSize, (__grid.height/2 - 1) * __cellSize)
        
        __tileObjectManager = TileObjectManager.new()
        __grid.setEventListener(__tileObjectManager)

        // ---
        // --- define all possible input actions and their key/button mappings
        // ---

        __inputActions = {}
        __inputActions["CursorLeft"]  = [ [Input.keyLeft, Input.keyA], [Input.gamepadDPadLeft] ]
        __inputActions["CursorRight"] = [ [Input.keyRight, Input.keyD], [Input.gamepadDPadRight] ]
        __inputActions["CursorUp"]    = [ [Input.keyUp, Input.keyW], [Input.gamepadDPadUp] ]
        __inputActions["CursorDown"]  = [ [Input.keyDown, Input.keyS], [Input.gamepadDPadDown] ]
        __inputActions["SelectCell"]  = [ [Input.keySpace], [Input.gamepadButtonEast] ]
    }

    /// Returns whether any of the keys/buttons associated to the given 'action' 
    /// have been pressed in this frame (if 'once' == true) or held down (if 'once' == false).
    static getInputForAction(action, once) {

        var possibleInputs = __inputActions[action]
        
        // keyboard keys
        for (key in possibleInputs[0]) {
            if (once) {
                if (Input.getKeyOnce(key)) return true
            } else {
                if (Input.getKey(key)) return true
            }
        }
        
        // gamepad buttons
        for (button in possibleInputs[1]) {
            if (once) {
                if (Input.getButtonOnce(button)) return true
            } else {
                if (Input.getButton(button)) return true
            }
        }

        return false
    }
    
    /// Executes the game logic for a single frame of the game loop. 
    static update(dt) {
        
        __gameTime = __gameTime + dt
        
        // --- update the moving tiles 

        var animationWasPlaying = !__playerCanMove
        __tileObjectManager.update(dt)

        // --- if the tiles have finished animating, check if something new needs to happen in the grid

        if (animationWasPlaying && __playerCanMove) {
            
            var chainsFound = __grid.replaceChains(3, __grid.emptyValue)
            if (chainsFound) {
                __grid.collapse()
            }
        }

        if (__playerCanMove) {
            
            // --- detecting directional input
            
            var move = false
            var nextX = __selectionX
            var nextY = __selectionY

            if (getInputForAction("CursorLeft", true) && __selectionX > 0) {
                move = true
                nextX = nextX - 1
            }
            if (getInputForAction("CursorRight", true) && __selectionX < __grid.width - 1) {
                move = true
                nextX = nextX + 1
            }
            if (getInputForAction("CursorUp", true) && __selectionY > 0) {
                move = true
                nextY = nextY - 1
            }
            if (getInputForAction("CursorDown", true) && __selectionY < __grid.height - 1) {
                move = true
                nextY = nextY + 1
            }

            // --- processing directional input

            if (move) {
                // safety net: a move can't be both horizontal and vertical at the same time
                if (__moveStarted && nextX != __selectionX && nextY != __selectionY) {
                    nextY = __selectionY
                }

                if (!__moveStarted) { 
                    // move the selector
                    __selectionX = nextX
                    __selectionY = nextY
                } else if (isMovePossible(nextX, nextY)) {
                    // swap two values in the grid
                    __grid.swapValues(__selectionX, __selectionY, nextX, nextY)
                    __selectionX = nextX
                    __selectionY = nextY
                    __moveStarted = false
                }
            }

            // --- starting/cancelling an action

            if (getInputForAction("SelectCell", true)) {
                if (!__moveStarted) {
                    var val = __grid.getValue(__selectionX, __selectionY)
                    if (val != __grid.emptyValue) {
                        __moveStarted = true
                        // check which moves are possible
                        __moveDirections[0] = isMovePossible(__selectionX, __selectionY - 1) 
                        __moveDirections[1] = isMovePossible(__selectionX + 1, __selectionY) 
                        __moveDirections[2] = isMovePossible(__selectionX, __selectionY + 1) 
                        __moveDirections[3] = isMovePossible(__selectionX - 1, __selectionY) 
                    }
                } else { 
                    __moveStarted = false
                }
            }
        }
    }

    static isMovePossible(x, y) {
        return __grid.isValidPosition(x,y) && __grid.getValue(x, y) != __grid.emptyValue
    }

    /// Computes and returns the screen position for the bottom-left(?) corner of the given grid cell.
    static gridToScreen(x, y) {
        return Vec2.new(x*__cellSize, -y*__cellSize) + __gridOffset
    }

    /// Renders the sprite with the given name at a screen position that matches a given grid cell.
    static spriteAtCell(name, x, y) {
        var pos = gridToScreen(x,y)
        Render.sprite(__sprites[name], pos.x, pos.y)
    }

    /// Renders the sprite with the given name at a screen position that matches a given grid cell, plus an offset vector.
    static spriteAtCell(name, x, y, offset) {
        var pos = gridToScreen(x,y) + offset
        Render.sprite(__sprites[name], pos.x, pos.y)
    }

    /// Renders all objects for a single frame of the game loop
    static render() {

        // --- draw a background sprite

        Render.sprite(__sprites["background"], -Data.getNumber("Width") / 2, -Data.getNumber("Height") / 2)
        
        // --- draw the grid background

        for (y in 0...__grid.height) {
            for (x in 0...__grid.width) {
                if ((x + y) % 2 == 0) {
                    spriteAtCell("cell0a", x, y)
                } else {
                    spriteAtCell("cell0b", x, y)
                }
            }
        }

        // --- draw grid content

        /*// old version: draw sprites directly based on the grid values
        for (y in 0...__grid.height) {
            for (x in 0...__grid.width) {
                var val = __grid.getValue(x,y)
                if (val != __grid.emptyValue) {
                    spriteAtCell("cell%(val)", x, y)
                }
            }
        }*/

        // new version: draw all tile objects, which manage their own movement through the screen
        __tileObjectManager.render()

        // --- draw the selector/arrows

        if (__moveStarted) {
            if (__moveDirections[0]) {
                spriteAtCell("arrow-up", __selectionX, __selectionY - 1)
            }
            if (__moveDirections[1]) {
                spriteAtCell("arrow-right", __selectionX + 1, __selectionY)
            }
            if (__moveDirections[2]) {
                spriteAtCell("arrow-down", __selectionX, __selectionY + 1)
            }
            if (__moveDirections[3]) {
                spriteAtCell("arrow-left", __selectionX - 1, __selectionY)
            }
        } else if (__playerCanMove) {
            spriteAtCell("selector", __selectionX, __selectionY, Vec2.new(-4, -4))
        }

    }
}