import "xs" for Render

class Game {
    static config() {
    }

    static init() {
        __img = Render.loadImage("[games]/examples/images/tree.png")
        //__img = Render.loadImage("[games]/shared/images/grey.png")
        __sprite = Render.createSprite(__img, 0, 0, 1, 1)
    }    
    
    static update(dt) {
    }

    static render() {
        Render.renderSprite(__sprite, 0, 0, 2.0, 0, 0xFFFFFFFF, 0x00000000, 0)
        Render.renderSprite(__sprite, 0, 0, 1.0, 0, 0xFFFF00FF, 0x00000000, 0)
        Render.renderSprite(__sprite, 0, 0, 1.0, 0, 0xFFFF00FF, 0x00000000, Render.spriteCenter)
        Render.renderSprite(__sprite, 0, 0, 0.5, 0, 0x000000FF, 0xFF0000FF, Render.spriteCenter)
    }
}
