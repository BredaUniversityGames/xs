
class Math {
    static lerp(a, b, t) { (a * (1.0 - t)) + (b * t) }
    static damp(a, b, lambda, dt) { lerp(a, b, 1.0 - (-lambda * dt).exp) }    
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
    ==(other) { (x == other.x) && (y == other.y) }
    !=(other) { !(this == other) }    
    magnitude { (x * x + y * y).sqrt }
    normalise { this / this.magnitude }
    dot(other) { (x * other.x + y * other.y) }
	cross(other) { }
    rotate(a) {
        _x = a.cos * _x - a.sin * _y
        _y = a.sin * _x + a.cos * _y
    }

    /*
    distanceTo(other) { Vector.distance(this, other) }
    static distance(a, b) {
        var xdiff = a.x - b.x
        var ydiff = a.y - b.y
        return ((xdiff * xdiff) + (ydiff * ydiff) ).sqrt
    }
    */

	toString { "[ %( this[0] ), %( this[1] ), ]" }		
}