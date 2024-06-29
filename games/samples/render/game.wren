import "xs" for Input, Render, Data
import "random" for Random
import "xs_math" for Math, Vec2

class Game {

    static config() {}

    static init() {
        __image = Render.loadImage("[games]/shared/images/icon_small.png")
        __sprite = Render.createSprite(__image, 0, 0, 1, 1)        

        var bg = Render.loadImage("[games]/shared/images/checkerboard_720p.png")
        __checkerboard = Render.createSprite(bg, 0, 0, 1, 1)

        __w = Data.getNumber("Width", Data.system) * 0.5
        __h = Data.getNumber("Height", Data.system) * 0.5
    }
    
    static update(dt) {
        Render.sprite(__checkerboard, -__w, -__h)

        if(Input.getKey(Input.keySpace)) {
            Render.setOffset(32, 32)
        } else {
            Render.setOffset(0, 0)
        }

        var x = -560        
        var y = 120
        var z = 0
        var dx = 200
        var dy = 200
        Render.sprite(__sprite, x, y, z)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteFlipX)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteFlipY)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteFlipX | Render.spriteFlipY) // |
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteOverlay)
        x = x + 1.5 * dx
        Render.sprite(__sprite, x, y, z, 1, Math.pi * 0.25, 0xffffffff, 0x00000000, Render.spriteOverlay)

        x = -560
        y = y - dy
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffff60, 0x00000000, 0)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0x00ffffff, 0x00000000, 0)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00ffff00, 0)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteTop)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteCenterX)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteCenterY)

        x = -560
        y = y - dy
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteCenterX | Render.spriteCenterY) 
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteCenter) 
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteCenter | Render.spriteFlipX | Render.spriteFlipY) 
        x = x + 2.5 * dx   
        y = y + 0.25 * dy
        Render.sprite(__sprite, x, y, z, 1.5, Math.pi * 0.25, 0xffffffff, 0x00000000, Render.spriteCenter | Render.spriteFlipX)
    }

    static render() { }
}
