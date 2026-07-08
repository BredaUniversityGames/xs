import "xs/ec" for Component

class DebugColor is Component {
    construct new(color) {
        super()
        _color = color
    }
    color { _color }
    color=(v) { _color = v }

    toString { "[DebugColor color:%(_color)]" }
}
