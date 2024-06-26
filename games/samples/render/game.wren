import "xs" for Input, Render, Data
import "random" for Random
import "xs_math" for Math, Vec2

class Game {

    static config() {}

    static init() {
        __image = Render.loadImage("[game]/xs.png")
        __sprite = Render.createSprite(__image, 0, 0, 1, 1)
        __x = 0
        __y = 0
    }
    
    static update(dt) {

        if(Input.getKey(Input.keyLeft)) __x = __x - 1
        if(Input.getKey(Input.keyRight)) __x = __x + 1



        Render.sprite(__sprite, __x, __y)


    }

    static render() { }
}
