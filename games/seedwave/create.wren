import "xs" for Configuration, Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor
import "components" for SlowRelation
import "random" for Random

class Create {
    
    static playerBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position + Vec2.new(0, 35))
        var v = Vec2.new(0, speed)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.Player, damage)
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
import "boss" for Boss
