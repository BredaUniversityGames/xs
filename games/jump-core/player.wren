import "xs" for Configuration, Input, Render, Registry
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite, Relation
import "globals" for Globals
import "tags" for Team, Tag
import "debug" for DebugColor

class Player is Component {
    construct new() {
        super()
        _shootTime = 0
        _drones = []
    }

    update(dt) {    
        // Keep in bounds
        var t = owner.getComponent(Transform)
        var h = Configuration.height * 0.5
        var w = Configuration.width * 0.5
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
        if((Input.getButton(0) || Input.getKeyOnce(Input.keySpace)) && _shootTime > 0.1) {
            Game.createBullet(owner, Globals.PlayerBulletSpeed, Globals.PlayerBulletDamage)
            for(d in _drones) {
                if(!d.deleted) {
                    Game.createBullet(d, Globals.PlayerBulletSpeed, Globals.PlayerBulletDamage)
                }
            }
            _shootTime = 0
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

        var s = owner.getComponent(GridSprite)
        if(vel.y > 0.5) {
            s.idx = 2
        } else if(vel.y > 0.2) {
            s.idx = 1
        } else if(vel.y < -0.5) {
            s.idx = 4
        } else if(vel.y < -0.2) {
            s.idx = 3
        } else {
            s.idx = 0
        }
        
        var speed = Globals.PlayerSpeed
        if(vel.magnitude > Globals.PlayerInputDeadZone) {            
            vel = vel * speed
        } else {
            vel = vel * 0
        }
        b.velocity = vel  
    }

    setLevel(level) {        
        var sheet = ""
        if(level == "daytime") {
            sheet = "[games]/jump-core/images/ships/blue-ship-spritesheet.png"
        } else if(level == "night") {
            sheet = "[games]/jump-core/images/ships/green-ship-spritesheet.png"
        } else if(level == "abandoned") {
            sheet = "[games]/jump-core/images/ships/emeraldgreen_ship_spritesheet.png"
        } else if(level == "snow-rain") {
            sheet = "[games]/jump-core/images/ships/darkgrey-ship-spritesheet.png"
        } else if(level == "sunset") {
            sheet = "[games]/jump-core/images/ships/orangered_ship_spritesheet.png"
        }
                 
        var s = GridSprite.new(sheet, 5, 1)
        s.layer = 1.0
        s.flags = Render.spriteCenter
        owner.addComponent(s)
    }

    static createShip() {
        var ship = Entity.new()            
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()
        var v = Vec2.new(0, 0)
        var b = Body.new( Globals.PlayerSize , v)
        var u = Unit.new(Team.player, Globals.PlayerHealth)
        var c = DebugColor.new(0x8BEC46FF)
        var o = Orbitor.new(ship)
        var s = GridSprite.new("[games]/jump-core/images/ships/blue-ship-spritesheet.png", 5, 1)
        s.layer = 1.0
        s.flags = Render.spriteCenter
        ship.addComponent(t)
        ship.addComponent(sc)            
        ship.addComponent(b)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(o)
        ship.addComponent(s)
        ship.name = "Player"
        ship.tag = (Tag.player | Tag.unit)
        {
            var thrust = Entity.new()
            var t = Transform.new(Vec2.new(0, 0))
            var s = AnimatedSprite.new("[games]/jump-core/images/ships/thrusters.png", 4, 2, 15)            
            s.layer = 0.999
            s.flags = Render.spriteCenter
            s.addAnimation("straight", [4, 5, 6, 7])            
            s.playAnimation("straight")
            s.scale = 2.0
            var r = Relation.new(ship)
            r.offset = Vec2.new(-20, 0)            
            thrust.addComponent(t)
            thrust.addComponent(s)
            thrust.addComponent(r)
        }

        return ship
    }

}

import "game" for Game
import "ships" for Orbitor, Shield, EnemyCore, Enemy
import "unit" for Unit
