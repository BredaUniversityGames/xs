import "xs/core" for Data, Input, Render
import "xs/math"for Math, Bits, Vec2, Color
import "xs/ec"for Entity, Component
import "xs/components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite
import "xs/tools" for Tools
import "random" for Random
import "types" for Type

/// This class is used to create entities in the game
/// by adding components to them
/// As a game programming pattern, it is a factory class
class Create {

    static initialize() {
        __random = Random.new()
        __id = 0

        // Create a list of all the types of monsters
        __monsterNames = {
            Type.mage: "Mage",
            Type.skeleton: "Skeleton",
            Type.ghost: "Ghost",
            Type.knight: "Knight",
            Type.crusader: "Crusader",
            Type.slime: "Slime",
            Type.dragon: "Dragon",
            Type.ogre: "Ogre"           
        }
        __monsterStats = {
            Type.mage: Stats.new(1, 1, 0, 0.4),
            Type.skeleton: Stats.new(1, 1, 0, 0.5),
            Type.ghost: Stats.new(2, 1, 0, 0.6),
            Type.knight: Stats.new(4, 1, 0, 1),
            Type.crusader: Stats.new(1, 1, 1, 0.4),
            Type.slime: Stats.new(2, 1, 0, 0.8),
            Type.dragon: Stats.new(5, 2, 0, 0.8),
            Type.ogre: Stats.new(3, 1, 0, 0.7)            
        }
        __itemNames = {
            Type.helmet: "Helmet",
            Type.armor: "Armor",
            Type.sword: "Sword",
            Type.food: "Food"
        }
        __itemStats = {
            Type.helmet: Stats.new(0, 0, 1, 0),
            Type.armor: Stats.new(0, 0, 2, 0),
            Type.sword: Stats.new(0, 1, 0, 0),
            Type.food: Stats.new(1, 0, 0, 0)
        }
    }

    static character(x, y) {
        var entity = Entity.new()
        var t = Transform.new(Level.calculatePos(x, y))
        var tl = Tile.new(x, y)
        var m = MoveAnimation.new()        
        entity.add(t)
        entity.add(tl)
        entity.add(m)
        return entity
    }

    static hero(x, y) {
        var entity = character(x, y)
        var h = Hero.new()
        entity.add(h)
        var s = Stats.new(10, 1, 0, 0)
        entity.add(s)
        var spr = AnimatedSprite.new("[game]/assets/tileset.xsanim")
        spr.playAnimation("hero")
        spr.layer = 1.0
        spr.mode = AnimatedSprite.loop
        spr.flags = Render.spriteCenter
        entity.add(spr)
        entity.tag = Type.player
        entity.name = "Hero"
        entity.enabled = false
        return entity
    }

    static monster(x, y) {
        var entity = character(x, y)
        var m = Monster.new()
        entity.add(m)
        var type = Tools.pickOne([
            Type.mage, Type.skeleton, Type.ghost,
            Type.knight, Type.crusader, Type.slime])
        var s = __monsterStats[type].clone()
        entity.add(s)
        var spr = AnimatedSprite.new("[game]/assets/tileset.xsanim")
        spr.playAnimation(__monsterNames[type])
        spr.layer = 1.0
        spr.flags = Render.spriteCenter
        spr.mode = AnimatedSprite.loop
        entity.add(spr)
        entity.tag = type
        entity.name = __monsterNames[type] + " " + Create.nextID.toString
        entity.enabled = false
        return entity
    }

    static item(x, y) {
        System.print("Creating item at " + x.toString + ", " + y.toString)
        var entity = Entity.new()
        var tl = Transform.new(Level.calculatePos(x, y))
        entity.add(tl)
        var t = Tile.new(x, y)
        entity.add(t)
        var type = Tools.pickOne([
            Type.helmet, Type.armor, Type.sword, Type.food])
        entity.tag = type
        entity.name = __itemNames[type] + " " + Create.nextID.toString
        var set = {
            Type.helmet : 108,
            Type.armor: 107,
            Type.sword: 102,
            Type.food: 100
        }
        var gs : GridSprite = GridSprite.new("[game]/assets/tileset.png", 16, 16)
        gs.idx = set[type]
        gs.layer = 100.0
        gs.flags = Render.spriteCenter
        entity.add(gs)
        var s = __itemStats[type].clone()
        entity.add(s)
        return entity
    }

    static background() {
        var entity = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        entity.add(t)
        var s = GridSprite.new("[game]/assets/tileset.png", 16, 16)
        s.idx = 0
        s.scale = 60
        s.flags = Render.spriteCenter
        entity.add(s)
        return entity
    }


    static nextID {
         __id = __id + 1
         return __id
    } 
}

import "gameplay" for Hero, Monster, Tile, Level, Stats, MoveAnimation