class Noise {
    // Maps integer grid coords to a float in [0, 1).
    // Uses the classic sin-based hash from GLSL — cheap and good enough for visuals.
    static hash(ix, iy) {
        var n = (ix * 127.1 + iy * 311.7).sin * 43758.5453
        return n.abs - n.abs.floor
    }

    // Smoothstep — makes interpolation ease in/out instead of going linear
    static smooth(t) { t * t * (3 - 2 * t) }

    // Value noise at continuous coords. Sample at e.g. (tx / 8, ty / 8)
    // to control the feature scale in tile-space.
    static value(x, y) {
        var ix = x.floor
        var iy = y.floor
        var fx = Noise.smooth(x - ix)
        var fy = Noise.smooth(y - iy)

        var a = Noise.hash(ix,     iy)
        var b = Noise.hash(ix + 1, iy)
        var c = Noise.hash(ix,     iy + 1)
        var d = Noise.hash(ix + 1, iy + 1)

        var ab = a + (b - a) * fx
        var cd = c + (d - c) * fx
        return ab + (cd - ab) * fy
    }

    // Fractal Brownian Motion — layers of noise at increasing frequencies.
    // More octaves = more detail. 3 is usually plenty.
    static fbm(x, y, octaves) {
        var val = 0
        var amp  = 0.5
        var freq = 1
        var norm = 0
        for (i in 0...octaves) {
            val  = val  + Noise.value(x * freq, y * freq) * amp
            norm = norm + amp
            amp  = amp  * 0.5
            freq = freq * 2
        }
        return val / norm
    }
}
