import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "boss" for Boss
import "components" for SlowRelation, TurnToVelocity

class Create {
    static bossHealthBar() {        
        var e = Entity.new()
        var t = Transform.new(Vec2.new(0, 180 - 14))
        var s = GridSprite.new("[game]/assets/images/ui/healthbar_boss.png", 1, 100)
        var b = Game.boss.getComponentSuper(Boss)
        var h = HealthBar.new(b)
        s.idx = 1
        s.layer = 100.0
        s.flags = Render.spriteCenter | Render.spriteOverlay // |
        e.addComponent(t)
        e.addComponent(s)
        e.addComponent(h)
        return e
    }

    static coreHealthBar() {
        var e = Entity.new()
        var t = Transform.new(Vec2.new(0, 180 - 24))
        var s = GridSprite.new("[game]/assets/images/ui/healthbar_core.png", 1, 100)
        var b = Game.boss.getComponentSuper(Unit)
        var h = HealthBar.new(b)
        s.idx = 1
        s.layer = 100.0
        s.flags = Render.spriteCenter | Render.spriteOverlay // |
        e.addComponent(t)
        e.addComponent(s)
        e.addComponent(h)
        return e
    }

    static playerHealthBar() {
        var e = Entity.new()
        var t = Transform.new(Vec2.new(0, -170))
        var s = GridSprite.new("[game]/assets/images/ui/healthbar_player.png", 1, 100)
        var u = Game.player.getComponentSuper(Unit)
        var h = HealthBar.new(u)
        s.idx = 1
        s.layer = 100.0
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

    static playerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0,10))
        var dir =  Vec2.new(Game.random.float(-0.2, 0.2), 1.0)
        dir = dir.normal
        dir = dir * speed
        var v = dir
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var tr = TurnToVelocity.new()
        var s = Sprite.new("[game]/assets/images/projectiles/pl_cannon.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        //s.addAnimation("anim", [0,1,2])
        //s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.addComponent(tr)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
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
        var bl = Missile.new(Team.Computer, speed)
        var u = Unit.new(Team.Computer, 1.0, true)
        var s = Sprite.new("[game]/assets/images/projectiles/missile.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        // s.addAnimation("anim", [0,1,2])
        // s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.addComponent(u)
        bullet.name = "Missile"
        bullet.tag = Tag.Computer | Tag.Bullet | Tag.Unit
        bullet.addComponent(DebugColor.new(0xFDFFC1FF))
    }

    static enemyBullet(owner, velocity, damage, offset){
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + offset)
        var v = null
        if(velocity is Vec2) {
            v = velocity
        } else if(velocity is Num) {
            v = Vec2.new(0, velocity)
        }
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var s = Sprite.new("[game]/assets/images/projectiles/cannon.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        //s.addAnimation("anim", [0,1,2])
        //s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFFA8D3FF))
    }

    static enemyVulcan(owner, velocity, damage, offset){
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + offset)
        var v = null
        var trn = TurnToVelocity.new()
        if(velocity is Vec2) {
            v = velocity
        } else if(velocity is Num) {
            v = Vec2.new(0, velocity)
        }
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
        var s = Sprite.new("[game]/assets/images/projectiles/vulcan.png")
        s.layer = 1.9
        s.flags = Render.spriteCenter
        //s.addAnimation("anim", [0,1,2])
        //s.playAnimation("anim") 
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.addComponent(trn)
        bullet.name = "Bullet"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xFFA8D3FF))
    }
}

import "player" for Player
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor
import "random" for Random
import "ui" for HealthBar
import "game" for Game
import "bullets" for Missile