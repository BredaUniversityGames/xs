import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "random" for Random 

class Create {
    static random {
        if(__random == null) {
            __random = Random.new()
        }
        return __random
    }

    static player() {
        var player = Entity.new()
        var t = Transform.new(Vec2.new(0,0))
        var p = Player.new()
        var b = Body.new(Data.getNumber("Player Size"), Vec2.new(0,0))
        //var u = Unit.new(Team.Player, Data.getNumber("Player HP"), true)
        // var c = DebugColor.new(0x8BEC46FF)
        var s = Sprite.new("[game]/assets/images/player.png")
        var sh = Shadow.new()
        s.layer = 2.0
        s.flags = Render.spriteCenter
        player.add(t)
        player.add(p)            
        player.add(b)
        //player.add(u)
        //player.add(c)
        player.add(s)
        player.add(sh)
        player.name = "Player"
        player.tag = (Tag.player)
        // Create.arrow(player)
        return player
    }

    static obstacle() {
        var radii = [50, 60, 70, 80, 90, 100, 110, 120]
        var radius = radii[Create.random.int(0, radii.count - 1)]
        var size = radius * 2

        //var sizeFrom = Data.getNumber("Obstacle Size From")
        //var sizeTo = Data.getNumber("Obstacle Size To")
        //var size = Create.random.float(sizeFrom, sizeTo)
        var w = Data.getNumber("World Width") * 0.5
        var h = Data.getNumber("World Height") * 0.5

        var pos = Vec2.new(random.float(-w, w), random.float(-h, h))
        var obstacle = Entity.new()
        var t = Transform.new(pos)
        var b = Body.new(size, Vec2.new(0,0))
        var r = Renderable.new()
        var s = Sprite.new("[game]/assets/images/disc_%(radius).png")
        var sh = Shadow.new()
        s.layer = 2.0
        s.flags = Render.spriteCenter
        s.mul = Data.getColor("Obstacle Color")
        obstacle.add(t)
        obstacle.add(b)
        obstacle.add(r)
        obstacle.add(s)
        obstacle.add(sh)
        obstacle.tag = (Tag.obstacle)
        return obstacle
    }
}

import "player" for Player
import "tags" for Tag
import "shadow" for Shadow 

/*
import "unit" for Unit

import "bullets" for Bullet, Missile, Laser
import "debug" for DebugColor
import "components" for SlowRelation, TurnToVelocity, TNB
import "random" for Random
import "ui" for HealthBar
import "arrow" for Arrow
import "game" for Game
import "monster" for Monster, Foot
import "damage" for Damage
import "aim" for Aim
*/
