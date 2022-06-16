import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor

class Create {

    static player() {
        var ship = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()
        var v = Vec2.new(0, 0)
        var b = Body.new(Registry.getNumber("Player Size"), v)
        var u = Unit.new(Team.player, Registry.getNumber("Player Health"))
        var c = DebugColor.new(0x8BEC46FF)
        var s = GridSprite.new("[games]/seedwave/assets/images/ships/planes_05_A.png", 4, 5)
        s.layer = 1.0
        s.flags = Render.spriteCenter
        ship.addComponent(t)
        ship.addComponent(sc)            
        ship.addComponent(b)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(s)
        ship.name = "Player"
        ship.tag = (Tag.Player | Tag.Unit)
        return ship
    }

    static playerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0, 35))
        var v = Vec2.new(0, speed)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.player, damage)
        var s = AnimatedSprite.new("[games]/seedwave/assets/images/projectiles/projectile-06-02.png", 3, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("anim", [0,1,2])
        s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }
}

import "player" for Player
