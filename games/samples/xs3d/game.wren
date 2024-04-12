import "xs" for Input, Render, Data, File, Audio

class Game {
    static config() {
    }

    static init() {
        var plane = Render.loadMesh("[game]/assets/plane.obj")
        var white = Render.loadTexture("[game]/assets/white.png")
        var box = Render.loadMesh("[game]/assets/box.obj")        
    }

    static update(dt) {
    }

    static render() {
        //Render.s
    }
}