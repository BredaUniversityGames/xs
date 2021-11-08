import "xs" for Configuration, Input, Render

class Game {

    static init() {
        var w = Configuration.width
        var h = Configuration.height

        System.print("Width=%(Configuration.width) Height=%(Configuration.height)")

        //Configuration.width = 512
        //Configuration.height = 512

        Configuration.title = "Squares"
        Configuration.multiplier = 2
        System.print("Width=%(Configuration.width) Height=%(Configuration.height)")
        
        __time = 0.0
        __x = 0.0
        __y = 0.0        
        __num = Num
    }    

    static sqr(x) {
        return x * x
    }

    static dotFunc(t, i, x, y) {        
        return (t - ((x-8).pow(2) + (y-8).pow(2)).sqrt).sin
    }
    
    static update(dt) {
        Render.setColor(1.0, 1.0, 0.0)
        Render.text("Hola@", 30, -20, 8)

        /*
        __time = __time + dt
        __x = __x + Input.getAxis(0)
        __y = __y + Input.getAxis(1)
        
        var bl = -256 + 16
        for (x in 0..15) {
            for(y in 0..15) {
                var i = x * 16 + y
                var r = dotFunc(__time, i, x, y)

                if(r > 0) {
                    Render.setColor(1, 1, 1)
                } else {
                    r = -r
                    Render.setColor(1, 0, 0)
                }
                if(r > 1) {
                    r = 1
                }
                r = r * 30.0
                Render.square(bl + x * 32, bl + y * 32, r)

                if(r > 5) {
                    Render.setColor(0, 0, 0)
                    r = r - 4.0
                    Render.square(bl + x * 32, bl + y * 32, r)
                }
            }
        } 
        */       
    }
}
