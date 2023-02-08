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

    static monster() {
        var monster = Entity.new()
        var t = Transform.new(Vec2.new(500, 450))
        var e = Monster.new()
        var b = Body.new(Data.getNumber("Part Size 0"), Vec2.new(0,0))
        var u = Unit.new(Team.Player, Data.getNumber("Monster HP") * 100, true)
        var c = DebugColor.new(0x8BEC46FF)
        var s = GridSprite.new("[game]/assets/images/parts.png", 7, 1)
        var ttv = TurnToVelocity.new()
        var tnb = TNB.new()
        s.idx = 0
        s.layer = 1.1
        s.flags = Render.spriteCenter
        monster.addComponent(t)
        monster.addComponent(e)            
        monster.addComponent(b)
        monster.addComponent(u)
        monster.addComponent(c)
        monster.addComponent(s)
        monster.addComponent(tnb)
        monster.name = "Monster"
        monster.tag = (Tag.Computer | Tag.Deflect | Tag.Unit)
        return monster
    }

    static shield(monster, offset) {
        var sh = Entity.new()
        var t = Transform.new(Vec2.new(0, 0))
        // var e = Monster.new()
        var b = Body.new(Data.getNumber("Shield Size"), Vec2.new(0,0))
        var u = Unit.new(Team.Player, Data.getNumber("Shield HP"), true)
        var c = DebugColor.new(0xFFEC46FF)
        var s = Sprite.new("[game]/assets/images/shield.png")
        var r = Relation.new(monster)
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

    static missile(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = Vec2.randomDirection()        
        v = v.normal * speed   
        if(v.y < 0) {
            v.y = -v.y
        }

        var bd = Body.new(5, v)
        var m = Missile.new(Team.Computer, speed)
        var u = Unit.new(Team.Computer, 1.0, true)
        var s = Sprite.new("[game]/assets/images//missile.png")
        var d = Damage.new(20) // TODO: Settings
        s.layer = 1.9
        s.flags = Render.spriteCenter
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(m)
        bullet.addComponent(s)
        bullet.addComponent(u)
        bullet.addComponent(d)
        bullet.name = "Missile"
        bullet.tag = Tag.Computer | Tag.Bullet | Tag.Unit
        bullet.addComponent(DebugColor.new(0xFDFFC1FF))
    }

    static laser(owner, damage) {
        var owt = owner.getComponent(Transform)
        var lsr = Entity.new()
        var t = Transform.new(owt.position - Vec2.new(0, 0))
        var v = Vec2.randomDirection()        
        var d = Damage.new(20) // TODO: Settings
        var l = Laser.new() // TODO: Settings
        var r = Relation.new(owner)
        lsr.addComponent(t)
        //lsr.addComponent(bd)
        //lsr.addComponent(m)
        //
        //lsr.addComponent(u)
        lsr.addComponent(d)
        lsr.addComponent(r)
        lsr.addComponent(l)
        lsr.name = "Laser"
        return lsr
        //bullet.tag = Tag.Computer | Tag.Bullet | Tag.Unit
        //bullet.addComponent(DebugColor.new(0xFDFFC1FF))
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

    static part(size, idx) {
        var part = Entity.new()
        var t = Transform.new(Vec2.new(Game.random.float(), Game.random.float()))
        var s = GridSprite.new("[game]/assets/images/parts.png", 7, 1)        
        var b = Body.new(size, Vec2.new(0,0))
        var u = Unit.new(Team.Computer, 1000000000, false)
        var tnb = TNB.new()
        s.layer = 0.2
        s.flags = Render.spriteCenter
        s.idx = idx
        part.addComponent(t)
        part.addComponent(s)        
        part.addComponent(b)
        part.addComponent(u)
        part.addComponent(tnb)
        part.addComponent(DebugColor.new(0xF0F0F0FF))
        part.name = "Part%(idx)"
        part.tag = (Tag.Computer | Tag.Deflect | Tag.Unit)
        return part        
    }

    static foot(parent, offset) {
        var foot = Entity.new()
        var t = Transform.new(Vec2.new())
        var s = Sprite.new("[game]/assets/images/foot.png")
        var f = Foot.new(parent, offset)
        var b = Body.new(15, Vec2.new(0,0))
        var u = Unit.new(Team.Computer, 1000000000, false)
        s.layer = 0.1
        s.flags = Render.spriteCenter
        foot.addComponent(t)
        foot.addComponent(s)
        foot.addComponent(f)
        foot.addComponent(b)
        foot.addComponent(u)
        foot.addComponent(DebugColor.new(0x00F0F0FF))
        foot.name = "Foot"
        foot.tag = (Tag.Computer | Tag.Deflect | Tag.Unit)
        return foot  
    }

    static hitbox(parent, offset, size) {
        var hb = Entity.new()
        var t = Transform.new(Vec2.new())
        var s = Sprite.new("[game]/assets/images/hitbox%(size).png")
        var r = Relation.new(parent)
        var b = Body.new(size, Vec2.new(0,0))
        var u = Unit.new(Team.Computer, 1000, true)
        s.layer = 0.09
        s.flags = Render.spriteCenter
        r.offset = offset
        hb.addComponent(t)
        hb.addComponent(s)
        hb.addComponent(r)
        hb.addComponent(b)
        hb.addComponent(u)
        hb.addComponent(DebugColor.new(0xFF3030FF))
        hb.name = "HitPoint"
        hb.tag = (Tag.Computer | Tag.Unit)
        return hb  
    }

}

import "player" for Player
import "unit" for Unit
import "tags" for Team, Tag
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
