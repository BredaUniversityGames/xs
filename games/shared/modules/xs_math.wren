///////////////////////////////////////////////////////////////////////////////
// Math tools
///////////////////////////////////////////////////////////////////////////////

import "random" for Random

class Math {
    static pi { 3.14159265359 }
    static lerp(a, b, t) { (a * (1.0 - t)) + (b * t) }
    static damp(a, b, lambda, dt) { lerp(a, b, 1.0 - (-lambda * dt).exp) }    
    static min(l, r) { l < r ? l : r }
    static max(l, r) { l > r ? l : r }

    static invLerp(a, b, v) {
	    var  t = (v - a) / (b - a)
	    t = max(0.0, min(t, 1.0))
	    return t
    }

    static remap(iF, iT, oF, oT, v) {
	    var t = invLerp(iF, iT, v)
	    return lerp(oF, oT, t)
    }

    static radians(deg) { deg / 180.0 * 3.14159265359 }
    static degrees(rad) { rad * 180.0 / 3.14159265359 }
    static mod(x, m)    { (x % m + m) % m }
}

class Bits {
    static switchOnBitFlag(flags, bit) { flags | bit }
    static switchOffBitFlag(flags, bit) { flags & (~bit) }
    static checkBitFlag(flags, bit) { (flags & bit) == bit }
    static checkBitFlagOverlap(flag0, flag1) { (flag0 & flag1) != 0 }
}

class Vec2 {
    construct new() {        
        _x = 0
        _y = 0
    }

    construct new(x, y) {
        _x = x
        _y = y
    }

    x { _x }
    y { _y }
    x=(v) { _x = v }
    y=(v) { _y = v }

    +(other) { Vec2.new(x + other.x, y + other.y) }
    -{ Vec2.new(-x, -y)}
    -(other) { this + -other }
    *(v) { Vec2.new(x * v, y * v) }
    /(v) { Vec2.new(x / v, y / v) }
    ==(other) { (other != null) && (x == other.x) && (y == other.y) }
    !=(other) { !(this == other) }    
    magnitude { (x * x + y * y).sqrt }
    normalise { this / this.magnitude }
    dot(other) { (x * other.x + y * other.y) }
	cross(other) { }
    rotate(a) {
        _x = a.cos * _x - a.sin * _y
        _y = a.sin * _x + a.cos * _y
    }
    rotated(a) {
        return Vec2.new(a.cos * _x - a.sin * _y,
                        a.sin * _x + a.cos * _y)
    }
    perp { Vec2.new(-y, x) }

    toString { "[%(_x), %(_y)]" }

    atan2 {
        // atan2 is an invalid operation when x = 0 and y = 0
        // but this method does not return errors.
        var a = 0.0
        if(_x > 0.0) {
            a = (_y / _x).atan
        } else if(_x < 0.0 && _y >= 0.0) {
            a = (_y / _x).atan + Math.pi
        } else if(_x < 0.0 && _y < 0.0) {
            a = (_y / _x).atan - Math.pi
        } else if(_x == 0 && _y > 0.0) {
            a = Math.pi / 2.0
        } else if(_x == 0 && _y < 0) {
            a = -Math.pi / 2.0
        }

        return a
    }


    static distance(a, b) {
        var xdiff = a.x - b.x
        var ydiff = a.y - b.y
        return ((xdiff * xdiff) + (ydiff * ydiff) ).sqrt
    }

    static distanceSq(a, b) {
        var xdiff = a.x - b.x
        var ydiff = a.y - b.y
        return ((xdiff * xdiff) + (ydiff * ydiff))
    }		

    static randomDirection() {
        if(__random == null) {
            __random = Random.new()
        }

        while(true) {
            var v = Vec2.new(__random.float(-1, 1), __random.float(-1, 1))
            if(v.magnitude < 1.0) {
                return v.normalise
            }
        }
    }

    static reflect(incident, normal) {
        return incident - normal * (2.0 * normal.dot(incident))
    }
}

class Color {
    construct new(r, g, b, a) {
        _r = r
        _g = g
        _b = b
        _a = a
    }
    construct new(r, g, b) {
        _r = r
        _g = g
        _b = b
        _a = 255
    }

    a { _a }
    r { _r }
    g { _g }
    b { _b }
    a=(v) { _a = v }
    r=(v) { _r = v }
    g=(v) { _g = v }
    b=(v) { _b = v }

    toNum { r << 24 | g << 16 | b << 8 | a }
    static fromNum(v) {
        var a = v & 0xFF
        var b = (v >> 8) & 0xFF
        var g = (v >> 16) & 0xFF
        var r = (v >> 24) & 0xFF
        return Color.new(r, g, b, a)
    }

    toString { "[r:%(_r) g:%(_g) b:%(_b) a:%(_a)]" }
}
