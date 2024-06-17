import "xs" for Render

class Tools {
    static toList(range) {
        var l = []
        for(i in range) {
            l.add(i)
        }
        return l
    }

    static toList(from, to) {
        var l = []
        for (i in from...to) {
            l.add(from + i)
        }
        return l
    }

    static removeFromList(list, element) {
        for(i in 0...list.count) {
            if(list[i] == element) {
                list.removeAt(i)
                return
            }        
        }
    }
}

class ShapeBuilder {
    construct new() {
        _position = []
        _texture = []
        _indices = []
    }    

    addPosition(position) {
        _position.add(position.x)
        _position.add(position.y)
    }

    addPosition(x, y) {
        _position.add(x)
        _position.add(y)
    }

    addTexture(texture) {
        _texture.add(texture.x)
        _texture.add(texture.y)
    }

    addTexture(x, y) {
        _texture.add(x)
        _texture.add(y)
    }

    addIndex(index) {
        _indices.add(index)
    }

    validate() {
        /*
        // Check if all arrays have the same length
        if( _positions.length != _normals.length ||
            _positions.length != _uvs.length ||
            _positions.length != _colors.length) {
            return false
        }

        // Check if the indices array is a multiple of 3 and if the positions array has the right size
        if( _indices.length % 3 != 0) {
            return false
        }
        */

        return true
    }

    build(image) {
        if(!validate()) return null
        var shape = Render.createShape(image, _position, _texture, _indices)
        return shape
    }
}
