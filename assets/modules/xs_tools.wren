import "xs" for Render
import "random" for Random

/// Utility functions and helpers for common operations
class Tools {
    /// Initializes the random number generator
    static initialize() {
        __random = Random.new()
    }

    /// Converts a range to a list
    static toList(range) {
        var l = []
        for(i in range) {
            l.add(i)
        }
        return l
    }

    /// Creates a list of numbers from 'from' to 'to'
    static toList(from, to) {
        var l = []
        for (i in from...to) {
            l.add(from + i)
        }
        return l
    }

    /// Removes the first occurrence of an element from a list
    static removeFromList(list, element) {
        for(i in 0...list.count) {
            if(list[i] == element) {
                list.removeAt(i)
                return
            }
        }
    }

    /// Returns a random element from the list
    static pickOne(list) {
        return list[__random.int(list.count)]
    }

    /// Gets the shared random number generator
    static random { __random }
}

/// Initialize the Tools class
Tools.initialize()

/// Builder class for creating custom mesh shapes
class ShapeBuilder {
    /// Creates a new empty ShapeBuilder
    construct new() {
        _position = []
        _texture = []
        _indices = []
    }

    /// Adds a position vertex from a Vec2
    addPosition(position) {
        _position.add(position.x)
        _position.add(position.y)
    }

    /// Adds a position vertex from coordinates
    addPosition(x, y) {
        _position.add(x)
        _position.add(y)
    }

    /// Adds a texture coordinate from a Vec2
    addTexture(texture) {
        _texture.add(texture.x)
        _texture.add(texture.y)
    }

    /// Adds a texture coordinate from UV values
    addTexture(x, y) {
        _texture.add(x)
        _texture.add(y)
    }

    /// Adds a triangle index
    addIndex(index) {
        _indices.add(index)
    }

    /// Validates that the shape data is correct
    validate() {
        // Check the arrays have same length
        if( _position.count != _texture.count)  return false

        // Check if the indices array is a multiple of 3 and if the positions array has the right size
        if( _indices.count % 3 != 0) return false
    
        // Check if the indices array is not out of bounds
        for(i in 0..._indices.count) {
            if(_indices[i] >= _position.count || _indices[i] < 0) return false
        }

        return true
    }

    /// Builds and returns the shape, or null if validation fails
    build(image) {
        if(!validate()) return null
        var shape = Render.createShape(image, _position, _texture, _indices)
        return shape
    }
}
