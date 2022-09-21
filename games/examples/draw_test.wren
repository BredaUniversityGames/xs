import "xs" for Input, Render
import "random" for Random
import "vector" for Vector, ColorRGBA, Base

class Math {

    static lerp(a, b, t) {
        return (a * (1.0 - t)) + (b * t) 
    }

    static damp(a, b, lambda, dt) {
        return lerp(a, b, 1.0 - (-lambda * dt).exp)
    }    
}

class Game {

    static init() {
        Data.getNumber("Width", Data.system) = 640
        Data.getNumber("Height", Data.system) = 360
        Configuration.title = "Draw Test"
        Configuration.multiplier = 2
        __time = 0.0
    }    
    
    static update(dt) {        
        __time = __time + dt

        // Clear screen
        Render.setColor(1, 1, 1)
        Render.rect(0, 0, Data.getNumber("Width", Data.system), Data.getNumber("Height", Data.system), 0.0)

        // Render three triangles
        var l = 120
        var m = 90
        var s = 70        
        Render.begin(Render.triangles)

            Render.setColor(1, 0, 0)
            Render.vertex(-l, -l)
            Render.setColor(0, 1, 0)
            Render.vertex(l, -l)
            Render.setColor(0, 0, 1)
            Render.vertex(0, l)

            Render.setColor(1, 0.5, 0.5)
            Render.vertex(-m, -m)
            Render.setColor(0.5, 1, 0.5)
            Render.vertex(m, -m)
            Render.setColor(0.5, 0.5, 1)
            Render.vertex(0, m)

            Render.setColor(1, 1, 0)
            Render.vertex(-s, -s)
            Render.setColor(1, 0, 1)
            Render.vertex(s, -s)
            Render.setColor(0, 1, 1)
            Render.vertex(0, s)

        Render.end()

        // Render some lines
        Render.setColor(1, 0, 0)
        var r = 150
        Render.line(0, 0, __time.sin * r, __time.cos * r)
        
        // Render lines strip
        var n = 128
        var light = ColorRGBA.new(1.0, 1.0, 1, 1)
        var dark = ColorRGBA.new(0.0, 0.0, 0.0, 1)

        Render.begin(Render.lines)        
            var t = 0.0     
            var f = 0.0   
            for(i in 0..(n+1)) {
                var x = t.sin * r
                var y = t.cos * r
                var color = Math.lerp(dark, light, f)
                Render.setColor(color.x, color.y, color.z)
                Render.vertex(x, y)
                t = t + (Num.pi * 2) / n
                f = f + (1.0 / n)
                f = f % 1
            }    
        Render.end()

        Render.vertex(-100, 0)
        Render.vertex(100, 0) 


        Render.setColor(0.5, 0.5, 0.5)
        Render.rect(0, 0, 10, 10, 0.0)
    }
}
