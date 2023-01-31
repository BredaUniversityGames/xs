import "xs" for Input, Render, Data
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2, Color
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "unit" for Unit
import "tags" for Team, Tag
import "bullets" for Bullet
import "debug" for DebugColor
import "components" for SlowRelation
import "random" for Random

class Player is Component {

    static create() {
        var ship = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()
        var v = Vec2.new(0, 0)
        var b = Body.new(Data.getNumber("Player Size"), v)
        var u = Unit.new(Team.Player, Data.getNumber("Player Health"), true)
        var c = DebugColor.new(0x8BEC46FF)
        var s = Sprite.new("[game]/assets/images/ships/player.png")
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

    construct new() {
        super()
        _shootTime = 0
        _animFrame = 0
        _frame = 0
    }

    update(dt) {
        // Get input        

        // Keep in bounds
        var t = owner.getComponent(Transform)
        var h = Data.getNumber("Height", Data.system) * 0.5
        var w = Data.getNumber("Width", Data.system) * 0.5
        if (t.position.x < -w) {
            t.position.x = -w
        } else if (t.position.x > w) {
            t.position.x = w
        }
        if (t.position.y < -h) {
            t.position.y = -h
        } else if (t.position.y > h) {
            t.position.y = h
        }

        _shootTime = _shootTime + dt
        if((Input.getButton(0) ||
            Input.getKey(Input.keySpace)) &&
            _shootTime > Data.getNumber("Player Shoot Time")) {
            Bullet.createPlayerBullet(
                    owner,
                    Data.getNumber("Player Bullet Speed"),
                    Data.getNumber("Player Bullet Damage"))
            _shootTime = 0
        }

        var speed = Data.getNumber("Player Speed")
        if(Input.getButton(Input.gamepadButtonWest)) {
            speed = Data.getNumber("Player Speed When Aiming")
        }

        var b = owner.getComponent(Body)
        var vel = Vec2.new(Input.getAxis(0), -Input.getAxis(1))
        if(Input.getKey(Input.keyUp)) {
            vel.y = 1.0
        }
        if(Input.getKey(Input.keyDown)) {
            vel.y = -1.0
        }
        if(Input.getKey(Input.keyRight)) {
            vel.x = 1.0
        }
        if(Input.getKey(Input.keyLeft)) {
            vel.x = -1.0
        }

        /*

        var s = owner.getComponent(GridSprite)
        
        _frame = _frame + 1
        if(_frame > 4) {
            _frame = 0
            _animFrame = (_animFrame + 1) % 4
        }


        if(vel.x > 0.5) {
            s.idx = 16
        } else if(vel.x > 0.35) {
            s.idx = 12
        } else if(vel.x < -0.5) {
            s.idx = 8
        } else if(vel.x < -0.35) {
            s.idx = 4
        } else {
            s.idx = 0
        }
        s.idx = s.idx + _animFrame
        */
        
        if(vel.magnitude > Data.getNumber("Player Input Dead Zone")) {            
            vel = vel * speed
        } else {
            vel = vel * 0
        }
        b.velocity = vel        
    }

    toString { "[Player]" }
}
