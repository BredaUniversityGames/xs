import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Evolver {
}

class Create {
    static BossHealthBar() {
        var e = Entity.new()
        var t = Transform.new(Vec2.new(0, 180 - 14))
        var s = GridSprite.new("[game]/assets/images/ui/healthbar_boss.png", 1, 100)
        var h = HealthBar.new(Game.boss.getComponent(Boss))
        s.idx = 1
        s.layer = 100.0
        s.mul = Color.new(255, 0, 128).toNum
        s.flags = Render.spriteCenter | Render.spriteOverlay // |
        e.addComponent(t)
        e.addComponent(s)      
        e.addComponent(h)          
        return e
    }
}

import "player" for Player
import "boss" for Boss
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor
import "components" for SlowRelation
import "random" for Random
import "ui" for HealthBar
import "game" for Game
