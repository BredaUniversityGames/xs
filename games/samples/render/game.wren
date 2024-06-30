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
        if(Input.getKey(Input.keySpace)) {
            Render.setOffset(32, 32)
        } else {
            Render.setOffset(0, 0)
        }
    }

    static cross(x, y) {
        Render.setColor(0x00ffffff)
        Render.line(x - 10, y, x + 10, y)
        Render.line(x, y - 10, x, y + 10)        
    }

    static render() {
        Render.sprite(__checkerboard, -__w, -__h, -1, Render.spriteOverlay)

        var x = -560        
        var y = 120
        var z = 0
        var dx = 200
        var dy = 200        
        var rot = Math.pi * 0.25
        var rot2 = rot * 0.25
        Render.sprite(__sprite, x, y, z)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteFlipX)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteFlipY)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteFlipX | Render.spriteFlipY) // |
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, Render.spriteOverlay)
        cross(x, y)
        x = x + 1.5 * dx
        Render.sprite(__sprite, x, y, z, 1, Math.pi * 0.25, 0xffffffff, 0x00000000, Render.spriteOverlay)
        cross(x, y)

        x = -560
        y = y - dy
        Render.sprite(__sprite, x, y, z, 1, rot2, 0xffffff60, 0x00000000, 0)
        cross(x, y)                
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, rot2, 0x00ffffff, 0x00000000, 0)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, rot2, 0xffffffff, 0x00ffff00, 0)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, rot2, 0xffffffff, 0x00000000, Render.spriteTop)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, rot2, 0xffffffff, 0x00000000, Render.spriteCenterX)
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, rot2, 0xffffffff, 0x00000000, Render.spriteCenterY)
        cross(x, y)

        x = -560
        y = y - dy
        Render.sprite(__sprite, x, y, z, 1, 0, 0xffffffff, 0x00000000, Render.spriteCenterX | Render.spriteCenterY) 
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, -rot, 0xffffffff, 0x00000000, Render.spriteCenter) 
        cross(x, y)
        x = x + dx
        Render.sprite(__sprite, x, y, z, 1, rot, 0xffffffff, 0x00000000, Render.spriteCenter | Render.spriteFlipX | Render.spriteFlipY) 
        cross(x, y)
        x = x + 2.5 * dx   
        y = y + 0.25 * dy
        Render.sprite(__sprite, x, y, z, 1.5, rot, 0xffffffff, 0x00000000, Render.spriteCenter | Render.spriteFlipX)
        cross(x, y)
    }
}
