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
    static clamp(a, f, t) { max(min(a, t), f) }
    static slerp(a,  b,  t) {
	    var CS = (1 - t) * (a.cos) + t * (b.cos)
	    var SN = (1 - t) * (a.sin) + t * (b.sin)
	    return Vec2.new(CS, SN).atan2
    }
    static sdamp(a, b, lambda, dt) { slerp(a, b, 1.0 - (-lambda * dt).exp) }    

    static quadraticBezier(a, b, c, t) {
        var ab = lerp(a, b, t)
        var bc = lerp(b, c, t)
        return lerp(ab, bc, t)
    }
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
    normal { this / this.magnitude }
    dot(other) { (x * other.x + y * other.y) }
	cross(other) { }
    rotate(a) {
        var x = _x
        _x = a.cos * _x - a.sin * _y
        _y = a.sin * x + a.cos * _y
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

    clear() {
     _x = 0
     _y = 0
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
                return v.normal
            }
        }
    }

    static reflect(incident, normal) {
        return incident - normal * (2.0 * normal.dot(incident))
    }

    static project(a, b) {
        var k = a.dot(b) / b.dot(b)
        return Vec2.new(k * b.x, k * b.y)
    }
}

class Geom {
    
    // Based on https://stackoverflow.com/questions/1073336/circle-line-segment-collision-detection-algorithm        
    static distanceSegmentToPoint(a, b, c) {
        // Compute vectors AC and AB
        var ac = c - a
        var ab = b - a

        // Get point D by taking the projection of AC onto AB then adding the offset of A
        var d = Vec2.project(ac, ab) + a

        var ad = d - a
        // D might not be on AB so calculate k of D down AB (aka solve AD = k * AB)
        // We can use either component, but choose larger value to reduce the chance of dividing by zero
        var k = ab.x.abs > ab.y.abs ? ad.x / ab.x : ad.y / ab.y

        // Check if D is off either end of the line segment
        if (k <= 0.0) {
            return Vec2.distance(c,a)
        } else if (k >= 1.0) {
            return Vec2.distance(c, b)
        }

        return Vec2.distance(c, d)
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

    +(other) { Color.new(r + other.r, g + other.g, b + other.b, a + other.a) }
    -(other) { Color.new(r - other.r, g - other.g, b - other.b, a - other.a) }
    *(other) {
        if(other is Color) {
            return Color.new(r * other.r, g * other.g, b * other.b, a * other.a)
        } else {
            return Color.new(r * other, g * other, b * other, a * other)
        }
    }

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
