import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "boss" for Boss

class Evolver {
}

class Create {
    static bossHealthBar() {
        var e = Entity.new()
        var t = Transform.new(Vec2.new(0, 180 - 14))
        var s = GridSprite.new("[game]/assets/images/ui/healthbar_boss.png", 1, 100)
        var b = Game.boss.getComponentSuper(Boss)
        var h = HealthBar.new(b)
        s.idx = 1
        s.layer = 100.0
        s.mul = Color.new(255, 0, 128).toNum
        s.flags = Render.spriteCenter | Render.spriteOverlay // |
        e.addComponent(t)
        e.addComponent(s)
        e.addComponent(h)
        return e
    }

    static explosion(owner) {
        var owt = owner.getComponent(Transform)
        var explosion = Entity.new()
        var t = Transform.new(owt.position)
        var s = AnimatedSprite.new("[game]/assets/images/fx/explosion.png", 16, 1, 60)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("explode", [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15])
        s.playAnimation("explode")
        s.mode = AnimatedSprite.destroy
        explosion.addComponent(t)
        explosion.addComponent(s)
        explosion.addComponent(DebugColor.new(0xFFFFFFFF))
        explosion.name = "Explosion"
    }
}

import "player" for Player
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor
import "components" for SlowRelation
import "random" for Random
import "ui" for HealthBar
import "game" for Game
