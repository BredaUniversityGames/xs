// Sample for debugging Wren scripts from VS Code.
//
// Open this folder in VS Code, set a breakpoint (click in the gutter, e.g. in
// Ball.update or Game.update) and press F5. Things to try:
//   - step over / in / out (F10, F11, Shift+F11)
//   - look at Locals, Closure and Module in the Variables view
//   - expand the "balls" list, then a ball to see its fields
//   - hover over a variable, or add a watch such as  __balls[0]._x
//   - press SPACE in the game to raise an error, the debugger stops on it
import "xs/core" for Render, Input, Data
import "xs/math" for Math, Color

class Ball {
    construct new(x, speed) {
        _x = x
        _y = 0
        _speed = speed
        _direction = 1
    }

    x { _x }

    update(dt) {
        var step = _speed * _direction * dt
        _x = _x + step
        if (_x > 560 || _x < -560) {
            _direction = -_direction
        }
    }

    render() {
        Render.sprite(Game.sprite, _x, _y, 0, 40, 0, 0xffffffff, 0, 0)
    }
}

class Game {
    static sprite { __sprite }

    static initialize() {
        var image = Render.loadImage("[shared]/images/white.png")
        __sprite = Render.createSprite(image, 0, 0, 1, 1)
        __balls = []
        for (i in 1..3) {
            __balls.add(Ball.new(-400 + 100 * i, 100 + 150 * i))
        }
        __stats = {"frames": 0, "name": "debug sample"}
    }

    static update(dt) {
        __stats["frames"] = __stats["frames"] + 1
        for (ball in __balls) {
            ball.update(dt)
        }
        if (Input.getKeyOnce(Input.keySpace)) {
            Fiber.abort("Space was pressed")
        }
    }

    static render() {
        for (ball in __balls) {
            ball.render()
        }
    }
}
