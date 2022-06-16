import "xs_ec"for Component

class DebugColor is Component {
    construct new(color) {
        super()
        _color = color
    }
    color { _color }
    color=(v) { _color = v}

    toString { "[Player color:%(_color)]" }
}