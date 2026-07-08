import "xs/core" for Render
import "xs/math" for Math, Vec2
import "xs/ec" for Entity, Component
import "xs/components" for Transform, GridSprite

// Renders a series of ghost sprites trailing behind entities that have this component.
// Each ghost independently damps its position toward the owner transform. Lower damp
// values follow more slowly, creating natural fall-off further from the entity.
class Trail is Component {
    construct new() {
        super()
        // Damp speeds per segment: index 0 = closest (fastest), index 5 = furthest (slowest)
        _damps     = [100, 75, 50, 35, 25, 18]
        _positions = null
    }

    initialize() {
        _transform = owner.get(Transform)
        var pos = _transform.position
        _positions = []
        for (i in 0...6) {
            _positions.add(Vec2.new(pos.x, pos.y))
        }
    }

    update(dt) {
        var target = _transform.position
        for (i in 0...6) {
            _positions[i] = Math.damp(_positions[i], target, _damps[i], dt)
        }
    }

    positions { _positions }

    static render() {
        // White with decreasing alpha: closest segment is most opaque
        var colors = [
            0xffffff99,  // 0 - closest
            0xffffff77,
            0xffffff55,
            0xffffff44,
            0xffffff33,
            0xffffff22,  // 5 - furthest
        ]

        for (e in Entity.entities) {
            var trail = e.get(Trail)
            var s     = e.get(GridSprite)
            var t     = e.get(Transform)
            if (trail == null || s == null || t == null) continue
            if (trail.positions == null) continue

            // Draw furthest first so closer ghosts composite on top
            var i = 5
            while (i >= 0) {
                Render.sprite(
                    s.sprite,
                    trail.positions[i].x,
                    trail.positions[i].y,
                    s.layer - 0.5,
                    s.scale,
                    t.rotation,
                    colors[i],
                    s.add,
                    s.flags
                )
                i = i - 1
            }
        }
    }

    toString { "[Trail]" }
}
