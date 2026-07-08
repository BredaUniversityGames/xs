import "xs/core" for Render
import "xs/ec" for Entity, Component
import "xs/math" for Vec2
import "xs/components" for Transform, Body

// Generic collision detection system.
//
// Usage:
//   cs.addPair(tagA, tagB)           -- detect overlaps between these two groups
//   cs.addPair(tagA, tagB, true)     -- also push overlapping entities apart
//   cs.on(tagA, tagB) { |a, b| ... } -- callback fired for each overlapping pair
//
// The system only does detection and resolution; all gameplay logic lives in
// the callbacks registered by the caller.
class CollisionSystem is Component {
    construct new() {
        super()
        _pairs    = []  // List of [tagA, tagB, resolve]
        _handlers = {}  // Map "tagA,tagB" -> Fn
    }

    // Register a tag pair to test for overlaps each frame
    addPair(tagA, tagB) { addPair(tagA, tagB, false) }
    addPair(tagA, tagB, resolve) {
        _pairs.add([tagA, tagB, resolve])
    }

    // Register a callback invoked with (entityA, entityB) on each overlapping pair
    on(tagA, tagB, fn) {
        _handlers["%(tagA),%(tagB)"] = fn
    }

    update(dt) {
        for (pair in _pairs) {
            checkPair(pair[0], pair[1], pair[2])
        }
    }

    checkPair(tagA, tagB, resolve) {
        var groupA  = Entity.withTag(tagA)
        var groupB  = Entity.withTag(tagB)
        var handler = _handlers["%(tagA),%(tagB)"]

        for (a in groupA) {
            if (a.deleted) continue
            var at = a.get(Transform)
            var ab = a.get(Body)
            if (at == null || ab == null) continue

            for (b in groupB) {
                if (b.deleted) continue
                var bt = b.get(Transform)
                var bb = b.get(Body)
                if (bt == null || bb == null) continue

                var diff    = at.position - bt.position
                var dist    = diff.magnitude
                var minDist = (ab.size + bb.size) * 0.5

                if (dist < minDist) {
                    if (resolve) {
                        var overlap = minDist - dist
                        var normal  = dist > 0 ? diff.normal : Vec2.new(1, 0)
                        at.position = at.position + normal * (overlap * 0.5)
                        bt.position = bt.position - normal * (overlap * 0.5)
                    }
                    if (handler != null) handler.call(a, b)
                }
            }
        }
    }

    toString { "[CollisionSystem]" }

    // Set to true to draw every Body as a circle each frame
    static debug=(v) { __debug = v }
    static debug      { __debug }

    static debugRender() {
        if (!__debug) return
        for (e in Entity.entities) {
            if (e.deleted) continue
            var t = e.get(Transform)
            var b = e.get(Body)
            if (t == null || b == null) continue
            Render.dbgColor(0x6000ff00)  // semi-transparent green (ARGB)
            Render.dbgDisk(t.position.x, t.position.y, b.size * 0.5, 16)
        }
    }
}

