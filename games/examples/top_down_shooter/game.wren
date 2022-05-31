import "xs" for Configuration, Input, Render
import "xs_math"for Math, Bits, Vec2

class EnemyShip {

}

class Ship {
    construct new() {
        var image = Render.loadImage("[games]/examples/top_down_shooter/images/spritesheets/ship.png")        
        var dc = 1.0 / 2.0
        var dr = 1.0 / 5.0
        _sprites = List.new()
        for(c in 0...2) {
            _sprites.add(List.new()) 
            for(r in 0...5) {                
                var s = Render.createSprite(image, r / 5.0, c / 2.0, (r+1) / 5.0, (c+1) / 2.0)
                _sprites[c].add(s)
            }
        }
        
        _time = 0.0           
        _pos = Vec2.new(0.0, -30.0)    
        _flip = 0 
    }

    update(dt) {
        _time = _time + dt
        _pos.x = _pos.x + Input.getAxis(0)
        _pos.y = _pos.y - Input.getAxis(1)
    }

    render() {
        _flip = _flip + 1
        _flip = _flip % 8
        var c = _flip < 4 ? 0 : 1        
        var turn = Input.getAxis(0)
        turn = Math.remap(-1.0, 1.0, 0.0, 4.0, turn).round
        var sprite = _sprites[c][turn]
        Render.renderSprite(sprite, _pos.x, _pos.y)
    }
}

class Game {
    static init() {        
        Configuration.width = 256
        Configuration.height = 360
        Configuration.title = "Top Down Shooter"
        Configuration.multiplier = 2
        System.print("Width=%(Configuration.width) Height=%(Configuration.height)")
        
        __time = 0.0
        __x = 0.0
        __y = 0.0        
        __num = Num
        __initialized = false
    }    

    static update(dt) {
        if(!__initialized) {
            var bgImage = Render.loadImage("[games]/examples/top_down_shooter/images/backgrounds/desert-backgorund.png")
            __bgSprite = Render.createSprite(bgImage, 0.0, 0.0, 1.0, 1.0)
            __bgY = 0.0

            var cloudsImage = Render.loadImage("[games]/examples/top_down_shooter/images/backgrounds/clouds-transparent.png")
            __cloudsSprite = Render.createSprite(cloudsImage, 0.0, 0.0, 1.0, 1.0)
            __clY = 0.0

            __ship = Ship.new()
        }
        __initialized = true

        __ship.update(dt)


        __bgY = __bgY - dt * 15
        Render.renderSprite(__bgSprite, -128, __bgY)
        Render.renderSprite(__bgSprite, -128, __bgY + 272)
        Render.renderSprite(__bgSprite, -128, __bgY - 272)
        if(__bgY < -272) {
            __bgY = __bgY + 272
        }            

        __clY = __clY - dt * 20
        Render.renderSprite(__cloudsSprite, -128, __clY)
        Render.renderSprite(__cloudsSprite, -128, __clY - 300)
        Render.renderSprite(__cloudsSprite, -128, __clY + 300)
        if(__clY < -300) {
            __clY = __clY + 300
        }            

        __ship.render()

        __time = __time + dt
        __x = __x + Input.getAxis(0)
        __y = __y + Input.getAxis(1)
    }
}
