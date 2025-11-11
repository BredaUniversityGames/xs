///////////////////////////////////////////////////////////////////////////////
// Math tools
///////////////////////////////////////////////////////////////////////////////

import "random" for Random

/// Mathematical utility functions
class Math {
    static pi { 3.14159265359 }

    /// Linear interpolation between two values
    /// Returns a value between a and b based on parameter t (0.0 to 1.0)
    static lerp(a, b, t) { (a * (1.0 - t)) + (b * t) }

    /// Smooth damping interpolation using exponential decay
    /// Useful for camera following and smooth movement
    static damp(a, b, lambda, dt) { lerp(a, b, 1.0 - (-lambda * dt).exp) }

    static min(l, r) { l < r ? l : r }
    static max(l, r) { l > r ? l : r }
    static atan2(y, x) { y.atan(x) }    
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
    static fmod(x, m)   { x - m * (x / m).floor }
    static clamp(a, f, t) { max(min(a, t), f) }    
    static slerp(a,  b,  t) {
	    var CS = (1 - t) * (a.cos) + t * (b.cos)
	    var SN = (1 - t) * (a.sin) + t * (b.sin)
	    return Vec2.new(CS, SN).atan2
    }
    static vslerp(a, b, t) {
        var omega = a.dot(b).acos
        var ta = ((1-t) * omega).sin / omega.sin
        var tb = (t * omega).sin / omega.sin
        return a * ta +  b * tb
    }
    static arc(a, b) {
        var diff = b - a
        if(diff > Math.pi) {
            diff = diff - Math.pi * 2
        } else if(diff < -Math.pi) {
            diff = diff + Math.pi * 2
        }
        return diff
    }
    static sdamp(a, b, lambda, dt) { slerp(a, b, 1.0 - (-lambda * dt).exp) }    

    static quadraticBezier(a, b, c, t) {
        var ab = lerp(a, b, t)
        var bc = lerp(b, c, t)
        return lerp(ab, bc, t)
    }

    static cubicBezier(a, b, c, d, t) {
        var ab = quadraticBezier(a, b, c, t)
        var bc = quadraticBezier(b, c, d, t)
        return lerp(ab, bc, t)
    }

    static catmullRom(a, b, c, d, t) {
        var t2 = t * t
        var t3 = t2 * t
        var a0 = d - c - a + b
        var a1 = a - b - a0
        var a2 = c - a
        return a0 * t3 + a1 * t2 + a2 * t + b        
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
    magnitudeSq { x * x + y * y }
    normal { this / this.magnitude }
    normalize() {
        _x = _x / magnitude
        _y = _y / magnitude
    }
    dot(other) { (x * other.x + y * other.y) }
	cross(other) { (x * other.y - y * other.x) }
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
    clear() {
        _x = 0
        _y = 0
    }    

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

    clamp(min, max) {
        _x = Math.clamp(_x, min.x, max.x)
        _y = Math.clamp(_y, min.y, max.y)
    }
    
    toString { "[%(_x), %(_y)]" }

    serialize { [_x, _y] }

    
    construct deserialize(data) {
        _x = data[0]
        _y = data[1]
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

    // Add two color represented as 32-bit integers (including alpha channel)
    static add(x, y) {
        var r = (x >> 24) + (y >> 24)
        var g = (x >> 16 & 0xFF) + (y >> 16 & 0xFF)
        var b = (x >> 8 & 0xFF) + (y >> 8 & 0xFF)
        var a = (x & 0xFF) + (y & 0xFF)
        r = r > 255 ? 255 : r
        g = g > 255 ? 255 : g
        b = b > 255 ? 255 : b
        a = a > 255 ? 255 : a
        return (r << 24) | (g << 16) | (b << 8) | a
    }

    // Multiply two color represented as 32-bit integers (including alpha channel)
    static mul(x, y) {
        var r = (x >> 24) * (y >> 24)
        var g = (x >> 16 & 0xFF) * (y >> 16 & 0xFF)
        var b = (x >> 8 & 0xFF) * (y >> 8 & 0xFF)
        var a = (x & 0xFF) * (y & 0xFF)
        //a = 255
        //r = r > 255 ? 255 : r
        //g = g > 255 ? 255 : g
        //b = b > 255 ? 255 : b
        //a = a > 255 ? 255 : a
        //return (r << 24) | (g << 16) | (b << 8) | a
        return a << 24 | b << 16 | g << 8 | r
    }

    toString { "[r:%(_r) g:%(_g) b:%(_b) a:%(_a)]" }
}
