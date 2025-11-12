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
        var h = Health.new(Data.getNumber("Player HP"))
        var s = Sprite.new("[game]/assets/images/player.png")
        var sh = Shadow.new()
        s.layer = 2.0
        s.flags = Render.spriteCenter
        player.add(t)
        player.add(p)            
        player.add(b)
        player.add(h)
        player.add(s)
        player.add(sh)
        player.name = "Player"
        player.tag = (Tag.player)
        return player
    }

    static enemy(position, target) {
        var enemy = Entity.new()
        var t = Transform.new(position)
        var e = Enemy.new(target)
        var b = Body.new(Data.getNumber("Enemy Size"), Vec2.new(0,0))
        var h = Health.new(Data.getNumber("Enemy HP"))
        h.enablePickupDrop(Data.getNumber("Pickup Value"))
        var s = Sprite.new("[game]/assets/images/enemy.png")
        var sh = Shadow.new()
        s.layer = 2.0
        s.flags = Render.spriteCenter
        s.mul = Data.getColor("Enemy Color")
        enemy.add(t)
        enemy.add(e)
        enemy.add(b)
        enemy.add(h)
        enemy.add(s)
        enemy.add(sh)
        enemy.name = "Enemy"
        enemy.tag = (Tag.enemy)
        return enemy
    }

    static pickup(position, value) {
        var pickup = Entity.new()
        var t = Transform.new(position)
        var p = Pickup.new(value)
        var b = Body.new(Data.getNumber("Pickup Size"), Vec2.new(0,0))
        var s = Sprite.new("[game]/assets/images/disc_50.png")
        s.layer = 1.5
        s.flags = Render.spriteCenter
        s.mul = Data.getColor("Pickup Color")
        s.scale = 0.3
        pickup.add(t)
        pickup.add(p)
        pickup.add(b)
        pickup.add(s)
        pickup.name = "Pickup"
        pickup.tag = (Tag.pickup)
        return pickup
    }

    static bullet(position, direction) {
        var bullet = Entity.new()
        var t = Transform.new(position)
        var speed = Data.getNumber("Bullet Speed")
        var bul = Bullet.new(direction, speed)
        var b = Body.new(Data.getNumber("Bullet Size"), Vec2.new(0,0))
        var s = Sprite.new("[game]/assets/images/bullet.png")
        s.layer = 1.0
        s.flags = Render.spriteCenter
        s.mul = Data.getColor("Bullet Color")
        bullet.add(t)
        bullet.add(bul)
        bullet.add(b)
        bullet.add(s)
        bullet.name = "Bullet"
        bullet.tag = (Tag.bullet)
        return bullet
    }

    static obstacle() {
        var radii = [50, 60, 70, 80, 90, 100, 110, 120]
        var radius = radii[Create.random.int(0, radii.count - 1)]
        var size = radius * 2

        var w = Data.getNumber("World Width") * 0.5
        var h = Data.getNumber("World Height") * 0.5

        var pos = Vec2.new(Create.random.float(-w, w), Create.random.float(-h, h))
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
import "enemy" for Enemy
import "bullet" for Bullet
import "health" for Health
import "pickup" for Pickup
import "tags" for Tag
import "shadow" for Shadow
