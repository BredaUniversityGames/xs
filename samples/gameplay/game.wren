import "xs" for Render, Data, Vector
import "xs_math" for Math, Color
import "xs_tools" for ShapeBuilder
import "xs_ec" for Entity, Component
import "xs_components" for Transform, Body, Renderable, Sprite
import "background" for Background
import "shadow" for Shadow

class Game {
    static config() {}

    static init() {
        Entity.init()        
        __time = 0
        //__image = Render.loadImage("[game]/images/gradient.png")
        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 40)
        __background = Background.new()

        __player = Create.player()
        for(i in 0...6) {
            var obstacle = Create.obstacle()
        }
    }    

    static update(dt) {
        __time = __time + dt
        __background.update(dt)
        // Body.update(dt)
        Entity.update(dt)
    }

    static render() {
        __background.render()
        Renderable.render()
        Shadow.render()
    }
}

import "create" for Create