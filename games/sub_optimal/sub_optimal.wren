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
        _drones = []
    }

    update(dt) {
        // Get input        

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
            for(d in _drones) {
                if(!d.deleted) {
                    Game.createBullet(d, 1000, 100)
                }
            }
            _shootTime = 0.0
        }

        var speed = 500.0
        if(Input.getButton(1) == true) {
            speed = speed * 0.4 
            var fromX = t.position.x
            var toX = t.position.x + 200
            var fromY = t.position.y - 30                
            var toY = t.position.y + 30
            Render.setColor("006400FF")
            Render.rect(fromX, fromY, toX, toY)
            var computerUnits = Entity.entitiesWithTag(Tag.Computer | Tag.Unit) //|
            for(cu in computerUnits) {
                var pos = cu.getComponent(Transform).position
                if(pos.x > fromX && pos.x < toX && pos.y > fromY && pos.y < toY) {                                        
                    var enemy = cu.getComponent(Enemy)
                    enemy.hack(dt)
                    if(enemy.hacked) {
                        cu.deleteComponent(Enemy)
                        cu.tag = Tag.Player | Tag.Unit //|
                        cu.addComponent(Drone.new())
                        cu.getComponent(DebugColor).color = "8BEC46FF"
                        addDrone2(cu)
                    }
                }
            }
        }

        var b = owner.getComponent(Body)
        var vel = Vec2.new(Input.getAxis(0), -Input.getAxis(1))
        if(vel.magnitude > 0.10) {            
            vel = vel * speed
        } else {
            vel = vel * 0.0
        }
        b.velocity = vel
    }

    addDrone(drone) {
        var idx = -1
        for(i in 0..._drones.count) {            
            if(_drones[i].deleted) {
                _drones[i] = drone
                idx = i
                break
            }
        }
        if(idx == -1) {
            idx = _drones.count            
            _drones.add(drone)
        }        
        idx = idx + 2
        var d = drone.getComponent(Drone)
        var sign = idx % 2 == 0 ? -1 : 1
        var y = (idx / 2).truncate * sign * 25
        var x = y * 0.5 * -sign
        d.offset = Vec2.new(x, y)
        System.print( "idx=%(idx) sign=%(sign) y=%(y) count=%(_drones.count) ")
    }

    addDrone2(drone) {
        _drones.add(drone)


        while(true) {
            var found = false
            for(i in 0..._drones.count) {            
                if(_drones[i].deleted) {
                    _drones.removeAt(i)
                    found = true
                    break
                }
            }
            if(found == false) {
                break
            }
        }

        for(i in 0..._drones.count) {
            var e = _drones[i]
            var d = e.getComponent(Drone)
            var ix = i + 2
            var sign = ix % 2 == 0 ? -1 : 1
            var y = (ix / 2).truncate * sign * 25
            var x = y * 0.5 * -sign
            d.offset = Vec2.new(x, y)
        }
    }
}

class Drone is Component {
    construct new() {
        _offset = Vec2.new(0.0, 0.0)
    }

    update(dt) {
        var shipPos = Game.playerShip.getComponent(Transform).position
        var toPos = shipPos + _offset
        var curPos = owner.getComponent(Transform).position
        owner.getComponent(Transform).position = Math.damp(curPos, toPos, 10.0, dt)
    }

    offset { _offset }
    offset=(v) { _offset = v }
}

class Enemy is Component {
    construct new(idx, pos, tilt) {
        super()        
        _position = pos
        _time = idx * 0.4
        _shootTime = _time
        _tilt = tilt
        _hack = 0.0
    }

    del() {
        Game.addScore(10)
    }

    update(dt) {
        _time = _time + dt * 2.0
        var pos = Vec2.new(30 * _time.sin, 70 * _time.cos)
        pos.rotate(_tilt)
        owner.getComponent(Transform).position = pos + _position
        _shootTime = _shootTime + dt
        if(_shootTime > 1.0) {
            if(Game.random.float(0.0, 1.0) < 0.3) {
                Game.createBullet2(owner, 200, 50)
            }
            _shootTime = 0.0
        }

        if(_hack > 0.0) {
            _hack = _hack - dt * 0.25
            Render.setColor("8BEC46FF")
            var pos = owner.getComponent(Transform).position
            Render.pie(pos.x, pos.y, 12, 2.0 * _hack * Num.pi / 0.3 ,32)
        }
    }

    hack(dt) { _hack = _hack + dt }
    hacked { _hack > 0.3 }
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
    color=(v) { _color = v}
}

///////////////////////////////////////////////////////////////////////////////
// Game
///////////////////////////////////////////////////////////////////////////////

class Menu {
    construct new(items) {
        _items = items
        _selected = 0
        _actions = {}
    }

    update(dt) {

        if(Input.getButtonOnce(13) == true) {
            _selected = (_selected + 1) % _items.count
        } else if (Input.getButtonOnce(11) == true) {
            _selected = (_selected - 1) % _items.count
        } else if (Input.getButtonOnce(0) == true) {
            var item = _items[_selected]
            var action = _actions[item]
            if(action != null) {
                action.call()
            }
        } 

        Render.setColor("8BEC46FF")
        var y = 55
        var x = -190
        Render.text("======== SubOptimal v0.1 ========", x, y, 2)
        var i = 0
        for(item in _items) {
            y = y - 20
            if(_selected == i) {
                Render.text(">" + item + "<", x, y, 2)
            } else {
                Render.text(item, x, y, 2)
            }            
            i = i + 1
        }
        y = y - 18
        Render.text("================================", x, y, 2)
    }

    addAction(name, action) {
        _actions[name] = action
    }
}

class GameState {
    static Menu     { 1 }
    static Play     { 2 }
    static Score    { 3 }
}

class Game {
    static init() {
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 1
        Configuration.title = "SubOptimal"

        Entity.init()
        //Enemy.init()

        __frame = 0
        __random = Random.new()
        __state = GameState.Menu        
        __score = 0
        __waveTimer = 0.0
        __wave = 0
        __menu = Menu.new(["play", "options", "credits", "exit"])
        __menu.addAction("play", Fn.new { Game.startPlay() } )
    }        
    
    static update(dt) {        
        Render.setColor(0.2, 0.2, 0.2)
        Render.rect(0, 0, Configuration.width, Configuration.height, 0.0)
        Entity.update(dt)

        if(__state == GameState.Menu) {
            __menu.update(dt)
        } else if(__state == GameState.Play) {
            updatePlay(dt)
        } else if(__state == GameState.Score) {
            updateScore(dt)
        } 
    }

    static startPlay() {
        Entity.init()
        __score = 0
        __waveTimer = 0.0
        __wave = 0
        __state = GameState.Play
        createPlayerShip()        
    }

    static updatePlay(dt) {
        Render.setColor("8BEC46FF")
        Render.text("SCORE %(__score)", -296, 140, 2)
        Render.text("WAVE %(__wave)", -296, 120, 2)

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

        if(playerShip.getComponent(Unit).health <= 0.0) {
            __state = GameState.Score
        }


        /*
        __frame = __frame + 1
        if(__frame == 100) {
            Entity.print()
            __frame = 0
        }
        */
    }
    static updateScore(dt) {
        Render.setColor("8BEC46FF")
        Render.text("SCORE %(__score)", -100, 50, 4)
        Render.text(">menu<", -50, 10, 2)

        if (Input.getButtonOnce(0) == true) {
            __state = GameState.Menu            
        } 
    }

    static createPlayerShip() {
        var ship = Entity.new()            
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()            
        var v = Vec2.new(0, 0)
        var b = Body.new(6, v)
        var u = Unit.new(Team.player, 1)
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
        var u = Unit.new(Team.computer, 1)
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
        //var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var v = Vec2.new(speed, 0)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.player, damage)
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.name = "Bullet"
        //if(owu.team == Team.player) {
            bullet.tag = Tag.Player | Tag.Bullet
            bullet.addComponent(DebugColor.new("8BEC46FF"))
        //} else {
        //    bullet.tag = Tag.Computer | Tag.Bullet
        //    bullet.addComponent(DebugColor.new("EC468BFF"))
        //}
    }

    static createBullet2(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var targetPos = playerShip.getComponent(Transform).position
        var dir = targetPos - owt.position
        var v = dir.normalise * speed
        var bd = Body.new(5, v)
        var bl = Bullet.new(owu.team, damage)
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.name = "Bullet2"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new("EC468BFF"))
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