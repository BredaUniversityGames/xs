System.print("start run game.wren import section")

import "xs" for Configuration, Input, Render
import "xs_math"for Math, Bits, Vec2
import "enemy" for Enemy

System.print("end run game.wren import section, start code")

class GameClass {
    construct new() {
        System.print("new")
        _score = 0
        _enemy = Enemy.new()
    }

    config() {
        Configuration.width = 256
        Configuration.height = 360
        Configuration.title = "Split Files"
        Configuration.multiplier = 2
        System.print("width=%(Configuration.width) height=%(Configuration.height)")
    }

    init() {
        System.print("init")        
        _initialized = true
    } 

    update(dt) {
        System.print("update(%(dt))")
        _enemy.update(dt)
        System.print("game.wren score=%(_score)")
    }

    render() {        
        //System.print("render")
    }

    addScore(s) {
        _score = _score + s
    }

    score { _score }

    initialized { _initialized }
}

var Game = GameClass.new()

System.print("end run game.wren")
