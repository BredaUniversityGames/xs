import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body
import "game" for Game, BulletType
import "globals" for Globals

class Orbitor is Component {

    construct new(parent) {
        super()
        _period = 2.0           
        _time = 0.0
        _tilt = 0.3
        _a = 30.0
        _b = 70.0
        _ships = []
        _parent = parent
    }

    update(dt) {
        var speed = Num.pi * 2.0 / _period
        _time = _time + dt * speed

        if(_ships.count > 0) {
            var delta = Num.pi * 2.0 / _ships.count
            var t = _time
            for(i in 0..._ships.count) {
                var ship = _ships[i]
                if(ship != null && !ship.deleted) {
                    var a = t + delta * i
                    var relPosition = Vec2.new(_a * a.sin, _b * a.cos)
                    relPosition.rotate(_tilt)
                    var parPosition = _parent.getComponent(Transform).position
                    ship.getComponent(Transform).position = relPosition + parPosition
                }
                t = t + delta
            }
        }
    }

    finalize() {
        for(ship in _ships) {
            if(ship != null) {
                ship.delete()
            }
        }
    }

    add(ship) {
        _ships.add(ship)
    }

    remove(ship) {
        _ships.remove(ship)
    }

    nullify(ship) {
        for(i in 0..._ships.count) {
            var s = _ships[i]
            if(s != null && s == ship) {
                _ships[i] = null
            }
        }
    }

    trim() {
        while(true) {
            var found = false
            for(i in 0..._ships.count) {            
                if(_ships[i].deleted) {
                    _ships.removeAt(i)
                    found = true
                    break
                }
            }
            if(found == false) {
                break
            }
        }
    }
}

class Shield is Component {
    update(dt) {}
}

class EnemyCore is Component {
    construct new(pos, tilt) {
        super()        
        _position = pos
        _backPos = pos + Vec2.new(400.0, 0.0)
        _time = 0.0
        _shootTime = _time
        _tilt = tilt 
    }

    finalize() { 
        Game.addScore(100)
    }

    update(dt) {

        var p = Game.playerShip
        var d = p.getComponent(Transform).position - owner.getComponent(Transform).position
        d = d.normal
        var b = owner.getComponent(Body)
        b.velocity = d * 20.0 

        /*
        _time = _time + dt * 0.5
        if(_time < 1.0) {
            var pos = Math.lerp(_backPos, _position, _time)
            owner.getComponent(Transform).position = pos
        } else {
            var pos = (_time - 1).sin * 20.0
            owner.getComponent(Transform).position.x = _position.x + pos
        }

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

class Enemy is Component {
    construct new(idx, tilt, parent, bulletType) {
        super()        
        _parent = parent
        _time = idx * 0.4
        _shootTime = _time
        _tilt = tilt
        _lock = 0
        _bulletType = bulletType
    }

    finalize() {
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
            if(Game.random.float(0, 1.0) < 0.3) {
                if(_bulletType == BulletType.straight) {
                    Game.createBulletStraightEnemy(
                        owner,
                        Globals.EnemyBulletSpeed,
                        Globals.EnemyBulletDamage)
                } else if(_bulletType == BulletType.directed) {
                    Game.createBulletDirectedEnemy(
                        owner,
                        Globals.EnemyBulletSpeed,
                        Globals.EnemyBulletDamage)
                } else if(_bulletType == BulletType.spread) {
                    Game.createBulletSpreadEnemy(
                        owner,
                        Globals.EnemyBulletSpeed,
                        Globals.EnemyBulletDamage)
                } else if(_bulletType == BulletType.follow) {
                    Game.createBulletDirectedEnemy(
                        owner,
                        Globals.EnemyBulletSpeed,
                        Globals.EnemyBulletDamage)
                }
            }
            _shootTime = 0
        }
    }

    parent { _parent }

    lock(dt) { _lock = _lock + dt }
    lock { _lock }
    locked { _lock > 1.0 }
}

