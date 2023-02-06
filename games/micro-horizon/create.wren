import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation

class Create {
    static background() {        
        var bg = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        var s = Sprite.new("[game]/assets/images/background.png")
        s.flags = Render.spriteCenter
        bg.addComponent(t)
        bg.addComponent(s)
        return bg
    }

    static player() {
        var player = Entity.new()
        var t = Transform.new(Vec2.new(-200,0))
        var p = Player.new()
        var b = Body.new(Data.getNumber("Player Size"), Vec2.new(0,0))
        var u = Unit.new(Team.Player, Data.getNumber("Player HP"), true)
        var c = DebugColor.new(0x8BEC46FF)
        var s = Sprite.new("[game]/assets/images/player.png")
        s.layer = 1.0
        s.flags = Render.spriteCenter
        player.addComponent(t)
        player.addComponent(p)            
        player.addComponent(b)
        player.addComponent(u)
        player.addComponent(c)
        player.addComponent(s)
        player.name = "Player"
        player.tag = (Tag.Player | Tag.Unit)
        Create.arrow(player)
        return player
    }

    static enemy() {
        var enemy = Entity.new()
        var t = Transform.new(Vec2.new(500, 450))
        var e = Enemy.new()
        var b = Body.new(Data.getNumber("Enemy Size"), Vec2.new(0,0))
        var u = Unit.new(Team.Player, Data.getNumber("Enemy HP"), true)
        var c = DebugColor.new(0x8BEC46FF)
        var s = Sprite.new("[game]/assets/images/core.png")
        s.layer = 1.1
        s.flags = Render.spriteCenter
        enemy.addComponent(t)
        enemy.addComponent(e)            
        enemy.addComponent(b)
        enemy.addComponent(u)
        enemy.addComponent(c)
        enemy.addComponent(s)
        enemy.name = "Enemy"
        enemy.tag = (Tag.Computer | Tag.Unit)
        return enemy
    }

    static shield(enemy, offset) {
        var sh = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        // var e = Enemy.new()
        var b = Body.new(Data.getNumber("Shield Size"), Vec2.new(0,0))
        var u = Unit.new(Team.Player, Data.getNumber("Shield HP"), true)
        var c = DebugColor.new(0xFFEC46FF)
        var s = Sprite.new("[game]/assets/images/shield.png")
        var r = Relation.new(enemy)
        s.layer = 1.1
        s.flags = Render.spriteCenter
        r.offset = offset
        sh.addComponent(t)
        // sh.addComponent(e)            
        sh.addComponent(b)
        sh.addComponent(u)
        sh.addComponent(c)
        sh.addComponent(s)
        sh.addComponent(r)
        sh.name = "Shield"
        sh.tag = (Tag.Computer | Tag.Unit | Tag.Deflect)
        return sh
    }

    static missile(enemy, offset) {
        var ms = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        // var e = Enemy.new()
        var b = Body.new(Data.getNumber("Missile Size"), Vec2.new(0,0))
        var u = Unit.new(Team.Player, Data.getNumber("Missile HP"), true)
        var c = DebugColor.new(0xC0FFFFFF)
        var s = Sprite.new("[game]/assets/images/missile.png")
        var r = Relation.new(enemy)
        s.layer = 1.1
        s.flags = Render.spriteCenter
        r.offset = offset
        ms.addComponent(t)
        // ms.addComponent(e)            
        ms.addComponent(b)
        ms.addComponent(u)
        ms.addComponent(c)
        ms.addComponent(s)
        ms.addComponent(r)
        ms.name = "Missile"
        ms.tag = (Tag.Computer)
        return ms
    }

    static arrow(owner) {
        var e = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        var a = Arrow.new()
        var s = AnimatedSprite.new("[game]/assets/images/arrow.png", 5, 1, 15)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("play", [1, 2, 3, 4])
        s.addAnimation("idle", [0])
        s.playAnimation("idle")
        s.mode = AnimatedSprite.once
        var r = Relation.new(owner)
        var d = Damage.new(Data.getNumber("Arrow HP"))
        s.layer = 1.9
        s.flags = Render.spriteCenter
        r.offset = Vec2.new(12.0, 0.0) 
        e.addComponent(t)
        e.addComponent(a)
        e.addComponent(s)
        e.addComponent(r)
        e.addComponent(d)
        e.name = "Arrow"
        //e.tag = Tag.Player | Tag.Bullet
        e.addComponent(DebugColor.new(0x8BEC46FF))
        return e
    }

    static bossHealthBar() {
        /*
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
        */
    }

    static explosion(owner) {
        var owt = owner.getComponent(Transform)
        var explosion = Entity.new()
        var t = Transform.new(owt.position)
        var s = AnimatedSprite.new("[game]/assets/images/explosion.png", 16, 1, 60)
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

    static warning(position) {
        var wrn = Entity.new()
        var t = Transform.new(position)
        var s = AnimatedSprite.new("[game]/assets/images/warning.png", 2, 1, 6)
        s.layer = 1.9
        s.flags = Render.spriteCenter
        s.addAnimation("play", [0, 1])
        s.playAnimation("play")
        s.mode = AnimatedSprite.loop
        wrn.addComponent(t)
        wrn.addComponent(s)
        wrn.addComponent(DebugColor.new(0xF0F0F0FF))
        wrn.name = "Warning"
        return wrn
    }

    static aim(owner) {
        var scp = Entity.new()
        var t = Transform.new(Vec2.new())
        var s = GridSprite.new("[game]/assets/images/aim.png", 8, 1)
        var r = Relation.new(owner)
        var a = Aim.new()
        s.layer = 0.1
        s.flags = Render.spriteCenter
        scp.addComponent(t)
        scp.addComponent(s)
        scp.addComponent(r)
        scp.addComponent(a)
        scp.addComponent(DebugColor.new(0xF0F0F0FF))
        scp.name = "Aim"
        return scp
    }

    static part(follow, dist, size) {
        var part = Entity.new()
        var t = Transform.new(Vec2.new())
        var s = Sprite.new("[game]/assets/images/missile.png")
        var p = Part.new(follow, dist)
        var b = Body.new(size, Vec2.new(0,0))
        s.layer = 0.1
        s.flags = Render.spriteCenter
        part.addComponent(t)
        part.addComponent(s)
        part.addComponent(p)
        part.addComponent(b)
        part.addComponent(DebugColor.new(0xF0F0F0FF))
        part.name = "Part"
        return part        
    }

    static foot(parent, offset) {
        var foot = Entity.new()
        var t = Transform.new(Vec2.new())
        var s = Sprite.new("[game]/assets/images/missile.png")
        var f = Foot.new(parent, offset)
        var b = Body.new(15, Vec2.new(0,0))
        s.layer = 0.1
        s.flags = Render.spriteCenter
        foot.addComponent(t)
        foot.addComponent(s)
        foot.addComponent(f)
        foot.addComponent(b)
        foot.addComponent(DebugColor.new(0x00F0F0FF))
        foot.name = "Foot"
        return foot  
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
import "arrow" for Arrow
import "game" for Game
import "enemy" for Enemy, Part, Foot
import "damage" for Damage
import "aim" for Aim
