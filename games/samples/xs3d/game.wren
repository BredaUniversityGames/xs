import "xs" for Input, Render, Data, File, Audio, Matrix, Vector
import "xs_math" for Math

class Game {
    static config() {
    }

    static init() {

        testVector()
        testMatrix()

        __plane = Render.loadMesh("[game]/assets/plane.obj")
        __white = Render.loadImage("[game]/assets/white.png")
        __box = Render.loadMesh("[game]/assets/box.obj")

        __transform = Matrix.new()
        __identity = Matrix.new()
        __x = 0
        __y = 0        

        __vZero = Vector.new(0, 0, 0, 0)
        __vWhite = Vector.new(1, 1, 1, 1)
        __vRed = Vector.new(1, 0, 0, 1)  

        __angle = 0      
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

        var lightDir = Vector.new(-1, -3, -2, 0)
        var lightColor = Vector.new(1, 1, 1, 1)
        Render.directionalLight(lightDir, lightColor)

        __angle = __angle + dt
        var r = 15.0
        var eye = Vector.new(r * __angle.cos, r * __angle.sin, 10, 0)
        var center = Vector.new(0, 0, 0, 0)
        var up = Vector.new(0, 0, 1, 0)
        var view = Matrix.new()
        view.lookAt(eye, center, up)
        Render.setView(view)
        var projection = Matrix.new()
        projection.perspective(Math.radians(45.0), 1280.0 / 720, 0.1, 100)
        Render.setProjection(projection)
    }

    static render() {        
        Render.mesh(__plane, __white, __identity, __vWhite, __vZero, 0)
        Render.mesh(__box, __white, __transform, __vRed, __vZero, 0)
    }

    static testVector() {
        // Test Vector

        // Test + operator
        var v = Vector.new(1,1,1,1) + Vector.new(1,2,3,4)
        System.print("Test + operator")
        System.print(v.list.toString)

        // Test - operator
        v = Vector.new(1,1,1,1) - Vector.new(1,2,3,4)
        System.print("Test - operator")
        System.print(v.list.toString)


        // Test * operator
        System.print("Test * operator")
        v = Vector.new(2,2,2,2) * Vector.new(1,2,3,4)   // Element-wise multiplication
        System.print(v.list.toString)
        v = Vector.new(2,2,2,2) * 2                      // Scalar multiplication
        System.print(v.list.toString)

        // Test / operator
        System.print("Test / operator")
        v = Vector.new(2,2,2,2) / Vector.new(1,2,3,4)   // Element-wise division
        System.print(v.list.toString)
        v = Vector.new(2,2,2,2) / 2                      // Scalar division
        System.print(v.list.toString)

        // Test dot product
        System.print("Test dot product")
        v = Vector.new(1,2,3,4)
        System.print(v.dot(Vector.new(1,2,3,4)))

        // Test cross product
        System.print("Test cross product")
        v = Vector.new(0,1,0,0)
        System.print(v.cross(Vector.new(4,2,3,1)).list.toString)

        // Test length
        System.print("Test length")
        v = Vector.new(1,2,3,4)
        System.print(v.length)

        // Test normalize
        System.print("Test normalize")
        v = Vector.new(1,2,3,4)
        System.print(v.normalize().list.toString)
        System.print(v.length)        

        // Test the element access
        v = Vector.new(1,2,3,4)
        System.print("Test the element access")
        System.print(v.x)
        System.print(v.y)
        System.print(v.z)
        System.print(v.w)
    }

    static testMatrix() {
        var m = Matrix.new()
        System.print(m.list)

    }
}