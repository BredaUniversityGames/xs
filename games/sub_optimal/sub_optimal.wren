import "xs" for Configuration, Input, Render
import "Assert" for Assert
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "random" for Random

///////////////////////////////////////////////////////////////////////////////
// Components
///////////////////////////////////////////////////////////////////////////////

class Tag {
    static None             { 0 << 0 }
    static Unit             { 1 << 1 }
    static Bullet           { 1 << 2 }
    static Player           { 1 << 3 }
    static Computer         { 1 << 4 }
}

class Transform is Component {
    construct new(position) {
         super()
        _position = position
    }

    position { _position }
    position=(p) { _position = p }

    toString { "[Transform position:%(_position)]" }
}

class Body is Component {    
    construct new(size, velocity) {
        super()
        _size = size
        _velocity = velocity
    }

    size { _size }
    velocity { _velocity }

    size=(s) { _size = s }
    velocity=(v) { _velocity = v }

    update(dt) {
        var t = owner.getComponent(Transform)
        t.position = t.position + _velocity * dt
    }

    toString { "[Body velocity:%(_velocity) size:%(_size)]" }
}

class Team {
    static player { 1 }
    static computer { 2 }
}

class Unit is Component {
    construct new(team, health) {
        super()
        _team = team
        _health = health
    }

    damage(d) { _health = _health - d }
    team { _team }
    health { _health }

    update(dt) {
        if(_health <= 0.0) {
            Game.createExplosion(owner)
            owner.delete()
        }
    }

    toString { "[Unit team:%(_team) health:%(_health)]" }
}

class Bullet is Component {
    construct new(team, damage) {
        super()
        _team = team
        _damage = damage
    }

    update(dt) {
        var t = owner.getComponent(Transform)
        var w = Configuration.width * 0.5
        if (t.position.x < -w) {
            owner.delete()
        } else if (t.position.x > w) {
            owner.delete()
        }
    }

    damage { _damage }
    team { _team }

    toString { "[Bullet team:%(_team) damage:%(_damage)]" }
}

class Player is Component {
    construct new() {
        super()
        _shootTime = 0.0
    }

    update(dt) {
        // Get input
        var b = owner.getComponent(Body)        
        var vel = Vec2.new(Input.getAxis(0), -Input.getAxis(1))
        if(vel.magnitude > 0.10) {            
            vel = vel * 500.0
        } else {
            vel = vel * 0.0
        }
        b.velocity = vel

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
        if(Input.getButton(0) == true && _shootTime > 0.1) {
            Game.createBullet(owner, 1000, 100)
            _shootTime = 0.0
        }

    }
}

class Enemy is Component {

    static init() {
        __idx = 0
    }

    construct new(idx, pos, tilt) {
        super()        
        _position = pos
        _time = idx * 0.4
        _shootTime = _time
        _tilt = tilt
    }

    del() {
        Game.addScore(10)
    }

    update(dt) {
        _time = _time + dt * 2.0
        var pos = Vec2.new(30 * _time.sin, 70 * _time.cos)
        pos.rotate(_tilt)
        owner.getComponent(Transform).position = pos + _position

        /*
        var toPos = Game.playerShip.getComponent(Transform).position
        var dir = toPos - owner.getComponent(Transform).position
        dir.x = 0.0
        dir.normalise
        var b = owner.getComponent(Body)
        b.velocity = dir * 1.5
        */

        _shootTime = _shootTime + dt
        if(_shootTime > 1.0) {
            Game.createBullet(owner, -200, 50)
            _shootTime = 0.0
        }
    }
}

class Explosion is Component {
    construct new(duration) {
        _time = 0.0
    }

    update(dt) {
        _time = _time + dt
        var b = owner.getComponent(Body)
        b.size =  _time.pow(0.06) * 15.0
        if(_time > 1) {
            owner.delete()
        }
    }
}

class DebugColor is Component {
    construct new(color) {
        super()
        _color = color
    }
    color { _color }
}

///////////////////////////////////////////////////////////////////////////////
// Game
///////////////////////////////////////////////////////////////////////////////

class Game {

    static init() {
        Entity.init()
        //Enemy.init()

        __frame = 0
        __random = Random.new()

        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 1
        Configuration.title = "SubOptimal"

        __score = 0
        __waveTimer = 0.0
        __wave = 1

        { // Create ship
            var ship = Entity.new()            
            var p = Vec2.new(0, 0)
            var t = Transform.new(p)
            var sc = Player.new()            
            var v = Vec2.new(0, 0)
            var b = Body.new(6, v)
            var u = Unit.new(Team.player, 1000)
            var c = DebugColor.new("8BEC46FF")
            ship.addComponent(t)
            ship.addComponent(sc)            
            ship.addComponent(b)
            ship.addComponent(u)
            ship.addComponent(c)
            ship.name = "Player"
            ship.tag = (Tag.Player | Tag.Unit)
            __ship = ship
        }

        createEnemyShips()
    }        
    
    static update(dt) {
        Entity.update(dt)

        Render.setColor(0.2, 0.2, 0.2)
        Render.rect(0, 0, Configuration.width, Configuration.height, 0.0)
        Render.setColor("8BEC46FF")
        Render.text("SCORE %(__score)", -296, 140, 2)

        for(e in Entity.entities) {
            var b = e.getComponent(Body)
            if(b != null) {
                var t = e.getComponent(Transform)
                var c = e.getComponent(DebugColor)
                if(c != null) {
                    Render.setColor(c.color)
                } else {
                    Render.setColor(1.0, 1.0, 1.0)
                }
                Render.disk(t.position.x, t.position.y, b.size, 24)
            }
        }

        var playerUnits = Entity.entitiesWithTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.entitiesWithTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.entitiesWithTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.entitiesWithTag(Tag.Computer | Tag.Bullet)

        Game.collide(computerBullets, playerUnits)
        Game.collide(playerBullets, computerUnits)

        if(computerUnits.count == 0) {
            __waveTimer = __waveTimer + dt            
        }

        if(__waveTimer >= 1.5) {
            for(w in 0..__wave) {
                createEnemyShips()
            }
            __waveTimer= 0.0
            __wave = __wave + 1
        }


        /*
        __frame = __frame + 1
        if(__frame == 100) {
            Entity.print()
            __frame = 0
        }
        */
    }

    static createEnemyShips() {
        var x = Game.random.float(0.0, 200.0)
        var y = Game.random.float(-100.0, 100.0)
        var pos = Vec2.new(x, y)
        var tilt = Game.random.float(0.0, 5.0)
        for(i in 0..6) {
            createEnemyShip(i, pos, tilt)
        }
    }

    static createEnemyShip(idx, pos, tilt) {
        var ship = Entity.new()
        var p = Vec2.new(240, 0)
        var t = Transform.new(p)    
        var v = Vec2.new(0, 0)
        var b = Body.new(10, v)
        var e = Enemy.new(idx, pos, tilt)
        var u = Unit.new(Team.computer, 200)
        var c = DebugColor.new("EC468BFF")
        ship.addComponent(t)
        ship.addComponent(b)
        ship.addComponent(e)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.name = "Enemy"
        ship.tag = (Tag.Computer | Tag.Unit)
    }

    static collide(bullets, units) {
        Render.setColor(1.0, 0.0, 0.0)
        for(u in units) {
            for(b in bullets) {            
                var uT = u.getComponent(Transform)
                var bT = b.getComponent(Transform)
                var uB = u.getComponent(Body)
                var bB = b.getComponent(Body)
                var dis = uT.position - bT.position
                dis = dis.magnitude
                if(dis < (uB.size + bB.size)) {
                    u.getComponent(Unit).damage(b.getComponent(Bullet).damage)
                    b.delete()
                    Render.disk(uT.position.x, uT.position.y, 2, 24)
                }
            }
        }
    }

    /*
    static render(dt) {  
    }
    */

    static playerShip { __ship }

    static random { __random }

    static addScore(s) { __score = __score + s }

    static createBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var v = Vec2.new(speed, 0)
        var bd = Body.new(5, v)
        var bl = Bullet.new(owu.team, damage)
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.name = "Bullet"
        if(owu.team == Team.player) {
            bullet.tag = Tag.Player | Tag.Bullet
            bullet.addComponent(DebugColor.new("8BEC46FF"))
        } else {
            bullet.tag = Tag.Computer | Tag.Bullet
            bullet.addComponent(DebugColor.new("EC468BFF"))
        }
    }

    static createExplosion(owner) {
        var owt = owner.getComponent(Transform)
        var explosion = Entity.new()
        var t = Transform.new(owt.position)
        var b = Body.new(0.001, Vec2.new(0.0, 0.0))
        var e = Explosion.new(1.0)
        explosion.addComponent(t)
        explosion.addComponent(b)
        explosion.addComponent(e)
        explosion.addComponent(DebugColor.new("FFFFFFFF"))
        explosion.name = "Explosion"
    }
}