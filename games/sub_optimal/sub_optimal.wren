import "xs" for Configuration, Input, Render, Registry, Color
import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "Assert" for Assert
import "random" for Random
import "globals" for Globals
import "components" for Transform, Body, Renderable, Sprite, GridSprite, AnimatedSprite
import "ships" for Orbitor, Shield

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

        var h = Configuration.height * 0.5
        if (t.position.y < -h) {
            owner.delete()
        } else if (t.position.y > h) {
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
        if((Input.getButton(0) || Input.getKeyOnce(Input.keySpace)) && _shootTime > 0.1) {
            Game.createBullet(owner, Globals.PlayerBulletSpeed, Globals.PlayerBulletDamage)
            for(d in _drones) {
                if(!d.deleted) {
                    Game.createBullet(d, Globals.PlayerBulletSpeed, Globals.PlayerBulletDamage)
                }
            }
            _shootTime = 0.0
        }

        var speed = Globals.PlayerSpeed
        if(Input.getButton(1)) {
            speed = Globals.PlayerSpeedWhenHacking
            var fromX = t.position.x
            var toX = t.position.x + Globals.PlayerHackWidth
            var fromY = t.position.y - Globals.PlayerHackHeight * 0.5
            var toY = t.position.y + Globals.PlayerHackHeight * 0.5
            Render.setColor(0x006400FF)
            Render.rect(fromX, fromY, toX, toY)
            var computerUnits = Entity.entitiesWithTag(Tag.Computer | Tag.Unit) //|
            for(cu in computerUnits) {
                var pos = cu.getComponent(Transform).position
                if(pos.x > fromX && pos.x < toX && pos.y > fromY && pos.y < toY) {                                        
                    var enemy = cu.getComponent(Enemy)
                    if(enemy != null) {
                        enemy.hack(dt)
                        if(enemy.hacked) {
                            cu.deleteComponent(Enemy)
                            cu.tag = Tag.Player | Tag.Unit //|
                            cu.addComponent(Drone.new())
                            cu.getComponent(DebugColor).color = Globals.PlayerColor
                            addDrone2(cu)
                            enemy.parent.getComponent(Orbitor).nullify(cu)
                        }
                    }
                }
            }
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

        if(vel.magnitude > Globals.PlayerInputDeadZone) {            
            vel = vel * speed
        } else {
            vel = vel * 0.0
        }
        b.velocity = vel
    }

    addDrone2(drone) {
        var o = owner.getComponent(Orbitor)
        o.trim()
        o.add(drone)

        /*
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
            var y = (ix / 2).truncate * sign * Globals.DroneSpread
            var x = y * 0.5 * -sign
            d.offset = Vec2.new(x, y)
        }
        */
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
    construct new(idx, tilt, parent) {
        super()        
        _parent = parent
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
        // var pos = Vec2.new(30 * _time.sin, 70 * _time.cos)
        // pos.rotate(_tilt)
        // var position = _parent.getComponent(Transform).position
        // owner.getComponent(Transform).position = pos + position
        _shootTime = _shootTime + dt
        if(_shootTime > 1.0) {
            if(Game.random.float(0.0, 1.0) < 0.3) {
                Game.createBulletEnemy(
                    owner,
                    Globals.EnemyBulletSpeed,
                    Globals.EnemyBulletDamage)
            }
            _shootTime = 0.0
        }

        if(_hack > 0.0) {
            _hack = _hack - dt * 0.25
            Render.setColor(Color.fromNum(0x8BEC46FF))
            var pos = owner.getComponent(Transform).position
            Render.pie(pos.x, pos.y, 12, 2.0 * _hack * Num.pi / 0.3 ,32)
        }
    }

    parent { _parent }

    hack(dt) { _hack = _hack + dt }
    hacked { _hack > 0.3 }
}

class EnemyCore is Component {
    construct new(pos, tilt) {
        super()        
        _position = pos
        _backPos = pos + Vec2.new(200.0, 0.0)
        _time = 0.0
        _shootTime = _time
        _tilt = tilt
    }

    del() {
        Game.addScore(100)
    }

    update(dt) {
        _time = _time + dt * 0.5
        if(_time < 1.0) {
            var pos = Math.lerp(_backPos, _position, _time)
            owner.getComponent(Transform).position = pos
        } else {
            var pos = (_time - 1).sin * 20.0
            owner.getComponent(Transform).position.x = _position.x + pos
        }

        /*
        _shootTime = _shootTime + dt
        if(_shootTime > 1.0) {
            if(Game.random.float(0.0, 1.0) < 0.3) {
                Game.createBulletEnemy(owner, 200, 50)
            }
            _shootTime = 0.0
        }
        */
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
        if(Input.getButtonOnce(13) == true || Input.getKeyOnce(Input.keyDown)) {
            _selected = (_selected + 1) % _items.count
        } else if (Input.getButtonOnce(11) == true || Input.getKeyOnce(Input.keyUp)) {
            _selected = (_selected - 1) % _items.count
        } else if (Input.getButtonOnce(0) == true || Input.getKeyOnce(Input.keySpace)) {
            var item = _items[_selected]
            var action = _actions[item]
            if(action != null) {
                action.call()
            }
        } 

        Render.setColor(0x000000FF)
        render(-190, 56)        
        Render.setColor(0xFFFFFFFF)
        render(-191, 57)                
    }

    render(x, y) {
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

class Parallax is Component {
    construct new(speed, width, repeat) {
        _speed = speed
        _width = width
        _repeat = repeat
    }

    update(dt) {        
        var t = owner.getComponent(Transform)
        t.position.x = t.position.x - dt * _speed

        if(t.position.x < Configuration.width * -0.5 -_width) {
            t.position.x = t.position.x + _width * _repeat
        }
    }

}

class Game {
    static config() {
        Configuration.width = 640
        Configuration.height = 360
        Configuration.multiplier = 1
        Configuration.title = "SubOptimal"
    }

    static init() {
        /*
        var col = Color.new(12, 255, 187, 67)
        System.print(col)
        System.print(col.toNum)
        col = Color.fromNum(218086211)
        System.print(col)
        col = Color.fromNum(0xCFFBB43)
        System.print(col)

        col = Color.new(12, 255, 187)
        System.print(col)
        System.print(col.toNum)
        col = Color.fromNum(218086399)
        System.print(col)
        col = Color.fromNum(0xCFFBBFF)
        System.print(col)
        col = Color.fromNum(0xCFFBB00)
        System.print(col)
        */

        Entity.init()
        createBackground()

        __frame = 0
        __random = Random.new()
        __state = GameState.Menu        
        __score = 0
        __waveTimer = 0.0
        __wave = 0
        __menu = Menu.new(["play", "options", "credits", "exit"])
        __menu.addAction("play", Fn.new { Game.startPlay() } )

        //var c =  Registry.getColor("some color")
        //System.print(c)

        // create
    }        
    
    static update(dt) {        
        // Render.setColor(0.2, 0.2, 0.2)
        // Render.rect(-Configuration.width, -Configuration.height, Configuration.width, Configuration.height)
        Entity.update(dt)

        if(__state == GameState.Menu) {
            __menu.update(dt)
        } else if(__state == GameState.Play) {
            updatePlay(dt)
        } else if(__state == GameState.Score) {
            updateScore(dt)
        } 
    }

    static render() {
        Renderable.render()
    }

    static startPlay() {
        // Entity.init()
        // __menu.delete()
        // __menu = null

        __score = 0
        __waveTimer = 0.0
        __wave = 0
        __state = GameState.Play
        createPlayerShip()        
    }

    static updatePlay(dt) {
        var pu = playerShip.getComponent(Unit)
        Render.setColor(0xFFFFFFFF)
        Render.text("SCORE %(__score)   |  WAVE %(__wave)  |  HEALTH %(pu.health)", -296, 170, 1)

        //Render.text("WAVE %(__wave)", -296, 120, 2)
        //Render.text("HEALTH %(pu.health)", -296, 100, 2)

        /*
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
        */

        var playerUnits = Entity.entitiesWithTag(Tag.Player | Tag.Unit)
        var playerBullets = Entity.entitiesWithTag(Tag.Player | Tag.Bullet)
        var computerUnits = Entity.entitiesWithTag(Tag.Computer | Tag.Unit)
        var computerBullets = Entity.entitiesWithTag(Tag.Computer | Tag.Bullet)
        // var computerBullets = Entity.entitiesWithTag(Tag.Computer | Tag.Bullet)

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
        Render.setColor(0x8BEC46FF)
        Render.text("SCORE %(__score)", -100, 50, 4)
        Render.text(">menu<", -50, 10, 2)

        if (Input.getButtonOnce(0) || Input.getKeyOnce(Input.keySpace)) {
            __state = GameState.Menu            
        } 
    }

    static createBackground() {
        var layers = [
            "[games]/sub_optimal/images/backgrounds/sky.png",
            "[games]/sub_optimal/images/backgrounds/far-buildings.png",
            "[games]/sub_optimal/images/backgrounds/buildings.png",
            "[games]/sub_optimal/images/backgrounds/trees.png"
        ]

        var widths = [
            368,
            176,
            416,
            208
        ]

        var speeds = [ 25.0, 40.0, 80.0, 120.0 ]

        var w = Configuration.width * 0.5
        var h = Configuration.height * 0.5

        for(i in 0...4) {
            for(j in 0..5) {
                var parallax = Entity.new()
                var t = Transform.new( Vec2.new(-w + widths[i] * j, -h))
                var s = Sprite.new(layers[i], 0.0, 0.0, 1.0, 1.0)
                var p = Parallax.new(speeds[i], widths[i], 5)
                parallax.addComponent(t)
                parallax.addComponent(s)
                parallax.addComponent(p)
            }
        }
    }

    static createPlayerShip() {
        var ship = Entity.new()            
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)
        var sc = Player.new()            
        var v = Vec2.new(0, 0)
        var b = Body.new( Globals.PlayerSize , v)
        var u = Unit.new(Team.player, Globals.PlayerHealth)
        var c = DebugColor.new(0x8BEC46FF)
        var o = Orbitor.new(ship)
        var s = AnimatedSprite.new("[games]/sub_optimal/images/ships/ship.png", 3, 1, 15)
        s.layer = 1.0
        s.anchor = Render.anchorCenter
        s.addAnimation("fly", [0,1,2])
        s.playAnimation("fly")
        ship.addComponent(t)
        ship.addComponent(sc)            
        ship.addComponent(b)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(o)
        ship.addComponent(s)
        ship.name = "Player"
        ship.tag = (Tag.Player | Tag.Unit)
        __ship = ship
    }

    static createEnemyShips() {
        var x = Game.random.float(0.0, 200.0)
        var y = Game.random.float(-100.0, 100.0)
        var pos = Vec2.new(x, y)
        var tilt = Game.random.float(0.0, 5.0)

        // "S"

        var core = createEnemyCore(pos, tilt)
        var orbitor = core.getComponent(Orbitor)
        for(i in 0..6) {
            var ship = createEnemyShip(i, tilt, core)
            orbitor.add(ship)
        }
    }

    static createEnemyCore(pos, tilt) {
        var core = Entity.new()
        var p = Vec2.new(0, 0)
        var t = Transform.new(p)    
        var v = Vec2.new(0, 0)
        var b = Body.new(Globals.EnemyCoreSize, v)
        var e = EnemyCore.new(pos, tilt)
        var u = Unit.new(Team.computer, Globals.EnemyCoreHealth)
        var c = DebugColor.new(Globals.EnemyColor)
        var o = Orbitor.new(core)
        var s = Sprite.new("[games]/sub_optimal/images/ships/spaceships.png",
                                4.0 / 5.0, 
                                1.0 / 3.0, 
                                5.0 / 5.0,
                                2.0 / 3.0)
        s.layer = 0.9
        s.anchor = Render.anchorCenter
        core.addComponent(t)
        core.addComponent(b)
        core.addComponent(e)
        core.addComponent(u)
        core.addComponent(c)
        core.addComponent(o)
        core.addComponent(s)
        core.name = "Enemy Core"
        core.tag = (Tag.Computer | Tag.Unit)
        return core
    }

    static createEnemyShip(idx, tilt, core) {
        var ship = Entity.new()
        var p = Vec2.new(240, 0)
        var t = Transform.new(p)    
        var v = Vec2.new(0, 0)
        var b = Body.new(Globals.EnemySize, v)
        var e = Enemy.new(idx, tilt, core)
        var u = Unit.new(Team.computer, Globals.EnemyHealth)
        var c = DebugColor.new(Globals.EnemyColor)
        var s = Sprite.new("[games]/sub_optimal/images/ships/spaceships.png",
                                2.0 / 5.0, 
                                1.0 / 3.0, 
                                3.0 / 5.0,
                                2.0 / 3.0)
        s.layer = 0.9
        s.anchor = Render.anchorCenter
        ship.addComponent(t)
        ship.addComponent(b)
        ship.addComponent(e)
        ship.addComponent(u)
        ship.addComponent(c)
        ship.addComponent(s)
        ship.name = "Enemy"
        ship.tag = (Tag.Computer | Tag.Unit)
        return ship
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
                    // Render.disk(uT.position.x, uT.position.y, 2, 24)
                }
            }
        }
    }

    static playerShip { __ship }

    static random { __random }

    static addScore(s) { __score = __score + s }

    static createBullet(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var v = Vec2.new(speed, 0)
        var bd = Body.new(5, v)
        var bl = Bullet.new(Team.player, damage)
        var s = Sprite.new("[games]/sub_optimal/images/projectiles/spark.png", 1.0/5.0, 0.0, 2.0/5.0, 1.0)
        s.layer = 0.9
        s.anchor = Render.anchorCenter

        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet"
        bullet.tag = Tag.Player | Tag.Bullet
        bullet.addComponent(DebugColor.new(0x8BEC46FF))
    }

    static createBulletEnemy(owner, speed, damage) {
        var owt = owner.getComponent(Transform)
        var owu = owner.getComponent(Unit)
        var bullet = Entity.new()
        var t = Transform.new(owt.position)
        var targetPos = playerShip.getComponent(Transform).position
        var dir = targetPos - owt.position
        var v = dir.normalise * speed
        var bd = Body.new(5, v)
        var bl = Bullet.new(owu.team, damage)
        var s = Sprite.new("[games]/sub_optimal/images/projectiles/Fx_04.png", 0.0/4.0, 0.0, 1.0/4.0, 1.0)
        s.layer = 0.9
        s.anchor = Render.anchorCenter
        bullet.addComponent(t)
        bullet.addComponent(bd)
        bullet.addComponent(bl)
        bullet.addComponent(s)
        bullet.name = "Bullet2"
        bullet.tag = Tag.Computer | Tag.Bullet
        bullet.addComponent(DebugColor.new(0xEC468BFF))
    }


    static createExplosion(owner) {
        var owt = owner.getComponent(Transform)
        var explosion = Entity.new()
        var t = Transform.new(owt.position)
        var b = Body.new(0.001, Vec2.new(0.0, 0.0))
        var e = Explosion.new(1.0)
        var s = AnimatedSprite.new("[games]/sub_optimal/images/vfx/Explosion.png", 8, 1, 15)
        s.layer = 1.9
        s.anchor = Render.anchorCenter
        s.addAnimation("explode", [0,1,2, 3, 4, 5, 6, 7])
        s.playAnimation("explode")
        s.mode = AnimatedSprite.destroy
        explosion.addComponent(t)
        explosion.addComponent(b)
        explosion.addComponent(e)
        explosion.addComponent(s)
        explosion.addComponent(DebugColor.new(0xFFFFFFFF))
        explosion.name = "Explosion"
    }
}