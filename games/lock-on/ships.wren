import "xs_ec"for Entity, Component
import "xs_math"for Math, Bits, Vec2
import "xs_components" for Transform, Body
import "game" for Game, BulletType
import "globals" for Globals

class Enemy is Component {
    construct new(bulletType, index, waveSize) {
        super()        
        _lock = 0
        _bulletType = bulletType
        //_index = index
        //_waveSize = waveSize
        _time = index / waveSize * 2 * Math.pi
        _shootTime = _time
    }

    finalize() {
        Game.addScore(10)
    }

    update(dt) {
        _time = _time + dt * 1.0

        var pos = Vec2.new(30 * _time.sin + 120, 80 * _time.cos)
        //var position = owner.getComponent(Transform).position
        owner.getComponent(Transform).position = pos

        _shootTime = _shootTime + dt
        if(_shootTime > 0.99) {
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
            _shootTime = 0
        }
    }
}

class Target is Component {
    construct new(lockSprite) {
        super()
        _lockSprite = lockSprite
        _lock = 0.0
    }

    update(dt) {
        if(_lock >= 1.0) {
            _lock = 1.0
        } else if(_lock <= 0.0) {
            _lock = 0.0
        } else  {
            _lock = _lock - dt * 0.25
        }
        _lockSprite.idx = (_lock * 32).floor
    }

    lock(dt) { _lock = _lock + dt * 2.0 }
    lock { _lock }
    locked { _lock >= 1.0 }
}
