import "xs" for Input, Render, Data
import "xs_math" for Vec2, Math
import "xs_ec"for Entity, Component
import "xs_components" for Renderable
import "xs_shapes" for Shapes, Shape, ShapeRenderer
import "random" for Random


class Game {
    static init() {
        Entity.init()
        __random = Random.new()
        var size = 100
        __quad = Shapes.quad(
            Vec2.new(0, 0),
            Vec2.new(size, 0),
            Vec2.new(size, size),
            Vec2.new(0, size),
            0xFFFFFFFF)

        __disk = Shapes.disk(   Vec2.new(100, 100),
                                50, 32, 0xFF00FFFF)
        __rect = Shapes.rectangle(  Vec2.new(200, 200),
                                    Vec2.new(300, 300),
                                    0xFF00FFFF)
        __randomConvexPoly = []
        for (i in 0...10) {
            var r = __random.float(50, 100)
            var a = i * Math.pi / 5.0
            var x = a.cos * r - 200
            var y = a.sin * r
            __randomConvexPoly.add(Vec2.new(x, y))
        }
        __fill = Shapes.fill(__randomConvexPoly, 0xFF00FFFF)
        __stroke = Shapes.stroke(__randomConvexPoly, 5.0, 0xFFFFFFFF)    

        __hex = Shapes.polygon(
            Vec2.new(0, -250),  // Center
            60,                 // Radius
            6,                  // Sides
            10,                 // Rouding radius
            5)                  // Rotation per rounding

        __hfill = Shapes.fill(__hex, 0x00FFFFFF)
        __hstroke = Shapes.stroke(__hex, 5.0, 0xFFFFFFFF)
    }

    static config() { }
    
    static update(dt) {
    }

    static render() {
        __rect.render(Vec2.new(0, 0), 1.0, 0.0)
        __quad.render(Vec2.new(0, 0), 1.5, 0.0)
        __disk.render(Vec2.new(0, 0), 1.0, 0.0)
        __fill.render(Vec2.new(0, 0), 1.0, 0.0)
        __stroke.render(Vec2.new(0, 0), 1.0, 0.0)

        __hfill.render(Vec2.new(0, 0), 1.0, 0.2)
        __hstroke.render(Vec2.new(0, 0), 1.0, 0.2)


        var points = [
            Vec2.new(100, 100),
            Vec2.new(200, 100),
            Vec2.new(200, 200),
            Vec2.new(100, 200),
            Vec2.new(100, 100)
        ]
        Game.drawLines(points, 0xFF0000FF)
        Game.drawLines(__randomConvexPoly, 0x00FF00FF)     
        Game.drawLines(__hex, 0x00FFFFFF)
    }

    static drawLines(points, color) {
        Render.begin(Render.lines)
        Render.setColor(color)
        for (p in points) {
            Render.vertex(p.x, p.y)    
        }
        Render.end()
    }
}