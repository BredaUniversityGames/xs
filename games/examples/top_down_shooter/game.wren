import "xs" for Configuration, Input, Render

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
        }
        __initialized = true


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

        __time = __time + dt
        __x = __x + Input.getAxis(0)
        __y = __y + Input.getAxis(1)        
    }    
}
