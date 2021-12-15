import "xs" for Configuration, Input, Render

class Game {

    static init() {
        
    }        
    
    static update(dt) {
        Render.setColor(1.0, 1.0, 1.0)

        // This is a comment. It does code-wise. It's for humans

        // We will store "Hol@ World" in a varaible (hence var)
        // we decided to call hola
        var hola = "Hol@ World"

        // Variables can store other things as well, like numbers
        var x = -96

        // We can change the value of vasriables too
        x = 0

        // We are going to render that text
        Render.text(hola, x, 0, 4)
    }
}
