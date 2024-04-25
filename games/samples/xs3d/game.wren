import "xs" for Input, Render, Data, File, Audio, Matrix, Vector

class Game {
    static config() {
    }

    static init() {
        __plane = Render.loadMesh("[game]/assets/plane.obj")
        __white = Render.loadImage("[game]/assets/white.png")
        __box = Render.loadMesh("[game]/assets/box.obj")

        __transform = Matrix.new()
        __identity = Matrix.new()
        __x = 0
        __y = 0
        var v = Vector.new(1,1,1,1)
        __vZero = Vector.new(0, 0, 0, 0)
        __vWhite = Vector.new(1, 1, 1, 1)
        __vRed = Vector.new(1, 0, 0, 1)
    }

    static update(dt) {
        // Move the box with the arrow keys
        if (Input.getKey(Input.keyLeft)) __x = __x - 0.1
        if (Input.getKey(Input.keyRight)) __x = __x + 0.1
        if (Input.getKey(Input.keyUp)) __y = __y + 0.1
        if (Input.getKey(Input.keyDown)) __y = __y - 0.1

        // Move the box with controller
        __x = __x - Input.getAxis(Input.gamepadAxisLeftStickX) * 0.2
        __y = __y + Input.getAxis(Input.gamepadAxisLeftStickY) * 0.2

        __transform.identity()
        __transform.translate(__x, __y, 0)
    }

    static render() {        
        Render.mesh(__plane, __white, __identity, __vWhite, __vZero, 0)
        Render.mesh(__box, __white, __transform, __vRed, __vZero, 0)
    }
}