import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Create {
    static player() {
        var player = Entity.new()
        var t = Transform.new(Vec2.new(-200,0))
        var p = Player.new()
        var b = Body.new(Data.getNumber("Player Size"), Vec2.new(0,0))
        //var u = Unit.new(Team.Player, Data.getNumber("Player HP"), true)
        // var c = DebugColor.new(0x8BEC46FF)
        var s = Sprite.new("[game]/assets/images/player.png")
        s.layer = 1.0
        s.flags = Render.spriteCenter
        player.addComponent(t)
        player.addComponent(p)            
        player.addComponent(b)
        //player.addComponent(u)
        //player.addComponent(c)
        player.addComponent(s)
        player.name = "Player"
        player.tag = (Tag.player)
        // Create.arrow(player)
        return player
    }
}

import "player" for Player
import "tags" for Tag

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
