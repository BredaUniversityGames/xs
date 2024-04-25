import "xs" for Input, Render, Data, File, Audio

class Game {
    static config() {
    }

    static init() {
        __plane = Render.loadMesh("[game]/assets/plane.obj")
        __white = Render.loadImage("[game]/assets/white.png")
        __box = Render.loadMesh("[game]/assets/box.obj")
    }

    static update(dt) {
    }

    static render() {
        Render.mesh(__plane, __white, 0, 0, 0, 0)
        Render.mesh(__box, __white, 0, 0, 0, 0)
    }
}