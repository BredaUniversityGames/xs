import "random" for Random

/// Mathematical utility functions for 2D game development
class Math {
    /// The mathematical constant pi
    static pi -> Num { 3.14159265359 }

    /// Linear interpolation between two values
    /// Returns a value between a and b based on parameter t (0.0 to 1.0)
    /// t=0 returns a, t=1 returns b, t=0.5 returns midpoint
    static lerp(a: Num, b: Num, t: Num) -> Num { (a * (1.0 - t)) + (b * t) }

    /// Smooth damping interpolation using exponential decay
    /// Useful for camera following and smooth movement - gives frame-rate independent smoothing
    /// Higher lambda values = faster convergence (try values like 5.0 to 20.0)
    static damp(a: Num, b: Num, lambda: Num, dt: Num) -> Num { lerp(a, b, 1.0 - (-lambda * dt).exp) }

    /// Returns the minimum of two values
    static min(l: Num, r: Num) -> Num { l < r ? l : r }
    /// Returns the maximum of two values
    static max(l: Num, r: Num) -> Num { l > r ? l : r }
    /// Returns the arctangent of y/x in radians
    static atan2(y: Num, x: Num) -> Num { y.atan(x) }
    /// Inverse linear interpolation - returns t such that lerp(a, b, t) = v
    static invLerp(a: Num, b: Num, v: Num) -> Num {
	    var  t = (v - a) / (b - a)
	    t = max(0.0, min(t, 1.0))
	    return t
    }

    /// Remaps a value from one range to another
    static remap(iF: Num, iT: Num, oF: Num, oT: Num, v: Num) -> Num {
	    var t = invLerp(iF, iT, v)
	    return lerp(oF, oT, t)
    }

    /// Converts degrees to radians
    static radians(deg: Num) -> Num { deg / 180.0 * 3.14159265359 }
    /// Converts radians to degrees
    static degrees(rad: Num) -> Num { rad * 180.0 / 3.14159265359 }
    /// Modulo operation that always returns positive results
    static mod(x: Num, m: Num) -> Num { (x % m + m) % m }
    /// Floating-point modulo operation
    static fmod(x: Num, m: Num) -> Num { x - m * (x / m).floor }
    /// Clamps a value between min (f) and max (t)
    static clamp(a: Num, f: Num, t: Num) -> Num { max(min(a, t), f) }
    /// Spherical linear interpolation for angles
    static slerp(a: Num, b: Num, t: Num) -> Num {
	    var CS = (1 - t) * (a.cos) + t * (b.cos)
	    var SN = (1 - t) * (a.sin) + t * (b.sin)
	    return Vec2.new(CS, SN).atan2
    }
    /// Vector spherical linear interpolation
    static vslerp(a: Vec2, b: Vec2, t: Num) -> Vec2 {
        var omega = a.dot(b).acos
        var ta = ((1-t) * omega).sin / omega.sin
        var tb = (t * omega).sin / omega.sin
        return a * ta +  b * tb
    }
    /// Returns the shortest angular distance between two angles
    static arc(a: Num, b: Num) -> Num {
        var diff = b - a
        if(diff > Math.pi) {
            diff = diff - Math.pi * 2
        } else if(diff < -Math.pi) {
            diff = diff + Math.pi * 2
        }
        return diff
    }
    /// Smooth damping for angles (spherical)
    static sdamp(a: Num, b: Num, lambda: Num, dt: Num) -> Num { slerp(a, b, 1.0 - (-lambda * dt).exp) }

    /// Quadratic Bezier curve interpolation
    static quadraticBezier(a: Num, b: Num, c: Num, t: Num) -> Num {
        var ab = lerp(a, b, t)
        var bc = lerp(b, c, t)
        return lerp(ab, bc, t)
    }

    /// Cubic Bezier curve interpolation
    static cubicBezier(a: Num, b: Num, c: Num, d: Num, t: Num) -> Num {
        var ab = quadraticBezier(a, b, c, t)
        var bc = quadraticBezier(b, c, d, t)
        return lerp(ab, bc, t)
    }

    /// Catmull-Rom spline interpolation
    static catmullRom(a: Num, b: Num, c: Num, d: Num, t: Num) -> Num {
        var t2 = t * t
        var t3 = t2 * t
        var a0 = d - c - a + b
        var a1 = a - b - a0
        var a2 = c - a
        return a0 * t3 + a1 * t2 + a2 * t + b
    }
}

/// Utility functions for bitwise operations and bit flags
class Bits {
    /// Turns on a specific bit flag
    static switchOnBitFlag(flags: Num, bit: Num) -> Num { flags | bit }
    /// Turns off a specific bit flag
    static switchOffBitFlag(flags: Num, bit: Num) -> Num { flags & (~bit) }
    /// Checks if all bits in 'bit' are set in 'flags'
    static checkBitFlag(flags: Num, bit: Num) -> Bool { (flags & bit) == bit }
    /// Checks if any bits overlap between two flags
    static checkBitFlagOverlap(flag0: Num, flag1: Num) -> Bool { (flag0 & flag1) != 0 }
}

/// 2D vector class for position, velocity, and direction calculations
/// Supports standard vector operations: addition, subtraction, scaling, dot product, cross product, rotation
class Vec2 {
    /// Creates a zero vector (0, 0)
    construct new() {
        _x = 0
        _y = 0
    }

    /// Creates a vector with the given x and y components
    construct new(x: Num, y: Num) {
        _x = x
        _y = y
    }

    /// Creates a vector with the same components as another vector
    construct new(other: Vec2) {
        _x = other.x
        _y = other.y
    }

    /// Gets the x component
    x -> Num { _x }
    /// Gets the y component
    y -> Num { _y }
    /// Sets the x component
    x=(v: Num) { _x = v }
    /// Sets the y component
    y=(v: Num) { _y = v }

    /// Adds two vectors
    +(other: Vec2) -> Vec2 { Vec2.new(x + other.x, y + other.y) }
    /// Negates the vector
    - -> Vec2 { Vec2.new(-x, -y)}
    /// Subtracts two vectors
    -(other: Vec2) -> Vec2 { this + -other }
    /// Multiplies vector by a scalar
    *(v: Num) -> Vec2 { Vec2.new(x * v, y * v) }
    /// Divides vector by a scalar
    /(v: Num) -> Vec2 { Vec2.new(x / v, y / v) }
    /// Checks if two vectors are equal
    ==(other) -> Bool { (other != null) && (x == other.x) && (y == other.y) }
    /// Checks if two vectors are not equal
    !=(other) -> Bool { !(this == other) }
    /// Gets the length of the vector
    magnitude -> Num { (x * x + y * y).sqrt }
    /// Gets the squared length of the vector (faster than magnitude)
    magnitudeSq -> Num { x * x + y * y }
    /// Returns a normalized copy of the vector (length = 1)
    normal -> Vec2 { this / this.magnitude }
    /// Returns a normalized copy of the vector (length = 1)
    normalized -> Vec2 { this / this.magnitude }
    /// Normalizes this vector in place
    normalize() {
        _x = _x / magnitude
        _y = _y / magnitude
    }
    /// Computes the dot product with another vector
    /// Returns a scalar: positive if vectors point same direction, negative if opposite, 0 if perpendicular
    dot(other: Vec2) -> Num { (x * other.x + y * other.y) }
    /// Computes the 2D cross product (returns scalar)
    /// Returns the z-component of the 3D cross product - useful for determining rotation direction
	cross(other: Vec2) -> Num { (x * other.y - y * other.x) }
    /// Rotates this vector by angle a (in radians) in place
    rotate(a: Num) {
        var x = _x
        _x = a.cos * _x - a.sin * _y
        _y = a.sin * x + a.cos * _y
    }
    /// Returns a rotated copy of this vector
    rotated(a: Num) -> Vec2 {
        return Vec2.new(a.cos * _x - a.sin * _y,
                        a.sin * _x + a.cos * _y)
    }
    /// Returns a perpendicular vector rotated 90 degrees counter-clockwise
    /// Useful for calculating normals and perpendicular directions
    perp -> Vec2 { Vec2.new(-y, x) }
    /// Sets the vector to (0, 0)
    clear() {
        _x = 0
        _y = 0
    }

    /// Returns the angle of this vector in radians
    atan2 -> Num {
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

    /// Clamps this vector's components between min and max
    clamp(min: Vec2, max: Vec2) {
        _x = Math.clamp(_x, min.x, max.x)
        _y = Math.clamp(_y, min.y, max.y)
    }

    /// Returns a string representation of this vector
    toString -> String { "[%(_x), %(_y)]" }

    /// Serializes the vector to a list
    serialize -> List { [_x, _y] }

    /// Creates a vector from serialized data
    construct deserialize(data: List) {
        _x = data[0]
        _y = data[1]
    }

    /// Computes the distance between two vectors
    static distance(a: Vec2, b: Vec2) -> Num {
        var xdiff = a.x - b.x
        var ydiff = a.y - b.y
        return ((xdiff * xdiff) + (ydiff * ydiff) ).sqrt
    }

    /// Computes the squared distance between two vectors (faster than distance)
    static distanceSq(a: Vec2, b: Vec2) -> Num {
        var xdiff = a.x - b.x
        var ydiff = a.y - b.y
        return ((xdiff * xdiff) + (ydiff * ydiff))
    }

    /// Returns a random unit direction vector
    static randomDirection() -> Vec2 {
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

    /// Reflects an incident vector off a surface with the given normal vector
    /// The normal should be normalized (unit length) for correct results
    static reflect(incident: Vec2, normal: Vec2) -> Vec2 {
        return incident - normal * (2.0 * normal.dot(incident))
    }

    /// Projects vector a onto vector b
    static project(a: Vec2, b: Vec2) -> Vec2 {
        var k = a.dot(b) / b.dot(b)
        return Vec2.new(k * b.x, k * b.y)
    }

    /// Creates a unit vector from an angle in radians
    static fromAngle(angle: Num) -> Vec2 {
        return Vec2.new(angle.cos, angle.sin)
    }

    /// Linear interpolation between two vectors
    static lerp(a: Vec2, b: Vec2, t: Num) -> Vec2 {
        return Vec2.new(Math.lerp(a.x, b.x, t), Math.lerp(a.y, b.y, t))
    }
}

/// Geometric utility functions
class Geom {

    /// Computes the distance from a line segment (a to b) to a point c
    /// Based on https://stackoverflow.com/questions/1073336/circle-line-segment-collision-detection-algorithm
    static distanceSegmentToPoint(a: Vec2, b: Vec2, c: Vec2) -> Num {
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

/// RGBA color class with utility functions
/// Color components are integers from 0-255
/// Supports arithmetic operations and conversion to/from 32-bit integers
class Color {
    /// Creates a color with RGBA components (0-255)
    construct new(r: Num, g: Num, b: Num, a: Num) {
        _r = r
        _g = g
        _b = b
        _a = a
    }
    /// Creates a color with RGB components (alpha defaults to 255)
    construct new(r: Num, g: Num, b: Num) {
        _r = r
        _g = g
        _b = b
        _a = 255
    }

    /// Gets the alpha component
    a -> Num { _a }
    /// Gets the red component
    r -> Num { _r }
    /// Gets the green component
    g -> Num { _g }
    /// Gets the blue component
    b -> Num { _b }
    /// Sets the alpha component
    a=(v: Num) { _a = v }
    /// Sets the red component
    r=(v: Num) { _r = v }
    /// Sets the green component
    g=(v: Num) { _g = v }
    /// Sets the blue component
    b=(v: Num) { _b = v }

    /// Adds two colors component-wise
    +(other: Color) -> Color { Color.new(r + other.r, g + other.g, b + other.b, a + other.a) }
    /// Subtracts two colors component-wise
    -(other: Color) -> Color { Color.new(r - other.r, g - other.g, b - other.b, a - other.a) }
    /// Multiplies color by another color or scalar
    *(other) {
        if(other is Color) {
            return Color.new(r * other.r, g * other.g, b * other.b, a * other.a)
        } else {
            return Color.new(r * other, g * other, b * other, a * other)
        }
    }

    /// Converts the color to a 32-bit integer (0xRRGGBBAA format)
    toNum -> Num { r << 24 | g << 16 | b << 8 | a }
    /// Creates a color from a 32-bit integer
    static fromNum(v: Num) -> Color {
        var a = v & 0xFF
        var b = (v >> 8) & 0xFF
        var g = (v >> 16) & 0xFF
        var r = (v >> 24) & 0xFF
        return Color.new(r, g, b, a)
    }

    /// Adds two colors represented as 32-bit integers (including alpha channel)
    static add(x: Num, y: Num) -> Num {
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

    /// Multiplies two colors represented as 32-bit integers (including alpha channel)
    static mul(x: Num, y: Num) -> Num {
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

    /// Returns a string representation of this color
    toString -> String { "[r:%(_r) g:%(_g) b:%(_b) a:%(_a)]" }
}
