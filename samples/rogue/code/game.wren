/// The imports of files that are used in this file usually go at the top.
/// If two files import each other (circular dependencies), then import go to the bottom of the file.
import "xs/core" for Data, Input, Render
//      ^       ^ class names that are imported
//      | file name without extension, relative to the this file (except shared modules and system libs)

import "xs/math"for Math, Bits, Vec2, Color
import "xs/ec"for Entity, Component
import "xs/components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite
import "types" for Type
import "directions" for Directions
import "gameplay" for Level
import "decorator" for Decorator

// There needs class called Game in you main file
class Game {

    // There are the states of the game
    // Wren does not have enums, so we use static variables
    static loading      { 0 }
    static generating   { 1 }
    static starting     { 2 }
    static playing      { 3 }
    static gameover     { 4 }

    // Initialize the game, which means initializing all the systems
    // and some variables that are used in the game`s logic
    static initialize() {
        Entity.initialize()
        Level.initialize()
        Tile.initialize()
        Create.initialize()
        Gameplay.initialize()
                
        __time = 0.0
        __state = generating // Skip loading

        var alg = Data.getNumber("Algorithm|Single Room|Randy|Random Walk|BSP")
        if(alg == 0) {
            __alg = SingleRoom
        } else if(alg == 1) {
            __alg = Randy
        } else if(alg == 2) {
            __alg = RandomWalk            
        } else if(alg == 3) {
            __alg = BSPer
        } else {
            System.print("Invalid algorithm number, using default")
            __alg = SingleRoom.new()
        }
        
        __genFiber =  Fiber.new {
            __alg.generate()
            Decorator.decorate()
        }
    }   
    
    // Update the game, which means updating all the systems
    static update(dt) {  
        if(__state == Game.generating) {
            genStep(dt)
        } else {
            Gameplay.update(dt)
        }

        Entity.update(dt)        
        __alg.debugRender()
    }

    // This function is called when the game is in the generating state
    // It is used to generate the level in steps, so the player can see the progress
    // it uses a fiber and a coroutine to do that. It's advance(ish) stuff, can be ignored
    static genStep(dt) {
        var visualize = Data.getBool("Visualize Generation", Data.game)
        if(visualize) {
            if(__time == null) {
                System.print("Error: __time is null, setting to 0")
                __time = 0.0
            }

            __time = __time - dt
            if(__time <= 0.0) {
                if(!__genFiber.isDone) {
                    __time = __genFiber.call()
                } else {
                    __state = Game.playing
                }
            }
        } else {
            while(!__genFiber.isDone) {
                __genFiber.call()
            } 
            __state = Game.playing
        }
    }

    // Render the game, which means rendering all the systems and entities
    static render() {    
        Gameplay.render()
    }
 }

/// Import classes from other files that might have circular dependencies (import each other)
import "create" for Create
import "generators" for SingleRoom, Randy, BSPer, RandomWalk // If you create a new Generator then add it's classname here 
import "gameplay" for Hero, Tile, Gameplay
