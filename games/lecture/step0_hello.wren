import "xs" for Input, Render

class Game {

    static init() {   
    }        
    
    static update(dt) {
        Render.setColor(1.0, 1.0, 1.0)
        Render.text("Hol@ World", -96, 0, 4)
    }
}
