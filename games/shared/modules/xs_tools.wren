import "xs" for Render

class Tools {
    static rangeToList(range) {
        var l = []
        for(i in range) {
            l.add(i)
        }
        return l
    }
}

class MeshBuilder {
    construct new() {
        _positions = []
        _normals = []
        _uvs = []
        _colors = []
        _indices = []
    }

    addPosition(position) {
        _positions.add(position.x)
        _positions.add(position.y)
        _positions.add(position.z)
    }

    addPosition(x, y, z) {
        _positions.add(x)
        _positions.add(y)
        _positions.add(z)
    }

    addNormal(normal) {
        _normals.add(normal.x)
        _normals.add(normal.y)
        _normals.add(normal.z)
    }

    addNormal(x, y, z) {
        _normals.add(x)
        _normals.add(y)
        _normals.add(z)
    }

    addUV(uv) {
        _uvs.add(uv.x)
        _uvs.add(uv.y)
    }

    addUV(x, y) {
        _uvs.add(x)
        _uvs.add(y)
    }

    addColor(color) {
        _colors.add(color.x)
        _colors.add(color.y)
        _colors.add(color.z)
        _colors.add(color.w)
    }

    addColor(r, g, b, a) {
        _colors.add(r)
        _colors.add(g)
        _colors.add(b)
        _colors.add(a)
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

    build() {
        if(!validate()) return null
        var mesh = Render.createMesh(_indices, _positions, _normals, _uvs, _colors)
        return mesh
    }
}