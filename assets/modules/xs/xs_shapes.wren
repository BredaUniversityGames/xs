/// doc-disable
///
/// xs_shapes
/// A module for rendering shapes and text. Made to bridge the
/// xs engine and the SVG Python tools.
/// NOTE: This API is currently not up-to-date and excluded from documentation
///

import "xs" for Render, File, Profiler
import "xs_math" for Vec2
import "xs_ec" for Entity, Component
import "xs_components" for Transform
import "xs_math" for Math, Color

/// A custom shape that can be rendered with vertices and colors
class Shape {
    /// Creates an empty shape
    construct new() {
        _points = []
        _colors = []
        _shape = null
    }

    /// Loads a shape from a CSV file with vertex data
    construct load(filename) {
        // _vertices = []
        // _colors = []
        var data = File.read(filename)        
        var lines = data.split("\n")        
        for(line in lines) {
            var parts = line.split(",")
            // System.print("Parts: %(parts.count)")
            if(parts.count == 3) {
                var x = Num.fromString(parts[0])
                var y = Num.fromString(parts[1])
                var color = Num.fromString(parts[2])
                vertex(Vec2.new(x, y), color)
            }
        }
    }

    /// Adds a vertex at the given position with the specified color
    vertex(pos, color) {
        vertex(pos.x, pos.y, color)
    }

    /// Adds a vertex at the given coordinates with the specified color
    vertex(x, y, color) {
        _points.add(x)
        _points.add(y)
        _colors.add(color)
    }

    /// Clears all vertices from the shape
    clear() {
        _points.clear()
        _colors.clear()
        // TODO: Delete shape
        _shape = null
    }

    /// Combines two shapes (currently returns empty shape)
    +(other) {
        var shape = Shape.new()
        /*
        for(i in 0..._vertices.count) {
            shape.vertex(_vertices[i], _colors[i])
        }
        for(i in 0...other.vertices.count) {
            shape.vertex(other.vertices[i], other.colors[i])
        }
        */
        return shape
    }

    /// Sets all vertices to the same color
    paint(color) {
        for(i in 0..._colors.count) _colors[i] = color
    }

    /// Renders the shape with default colors
    render(position, scale, rotation) {
        render(position, scale, rotation, 0xffffffff, 0x00000000)
    }

    /// Renders the shape with the given transformation and colors
    render(position, scale, rotation, mulColor, addColor) {
        if(!_shape) _shape = Render.createShape(_points, _colors)
        Render.shape(
            _shape,
            position.x,
            position.y,
            scale,
            rotation,
            mulColor,
            addColor)
    }

    /// Gets the colors array
    colors { _colors }
}

/// Represents a single glyph (character) with its shape and advance width
class Glyph {
    /// Creates a glyph with the given shape and horizontal advance
    construct new(shape, advance) {
        _shape = shape
        _advance = advance
    }

    /// Gets the glyph shape
    shape { _shape }
    /// Gets the horizontal advance width
    advance { _advance }
}

/// Font class that stores glyphs for text rendering
class Font {
    /// Loads a font from a file
    construct load(filename) {
        _glyphs = {}
        var data = File.read(filename)
        var lines = data.split("\n")                
        for(line in lines) {
            // Early out if the line is empty
            if(line.count == 0) continue

            // Check if the first character is the delimiter ':':            
            var parts = []
            if(line[0] == ":") {                                
                parts = line.split(":")
                parts.removeAt(0)       // Skip the first character
                parts[0] = ":"
            } else {
                parts = line.split(":")                
            }

            var letter = parts[0]
            var advance = Num.fromString(parts[1])
            var shape = Shape.new()

            for(i in 2...parts.count) {
                if(parts[i].count == 0) continue
                var vertex = parts[i].split(",")

                var x = Num.fromString(vertex[0])
                var y = Num.fromString(vertex[1])
                var color = 0xFFFFFFFF
                shape.vertex(Vec2.new(x, y), color)
            }
            _glyphs[letter] = Glyph.new(shape, advance)  
        }
    }

    /// Gets the glyphs dictionary
    glyphs { _glyphs }
}

/// Component for rendering a shape on an entity
class ShapeRenderer is Component {

    /// Creates a ShapeRenderer with default layer 0
    construct new(shape) {
        _layer = 0
        _shape = shape
        _addColor = 0
        _mulColor = 0xFFFFFFFF
    }

    /// Creates a ShapeRenderer with a specific layer
    construct new(shape, layer) {
        _layer = layer
        _shape = shape
        _addColor = 0
        _mulColor = 0xFFFFFFFF

    }

    /// Initializes by getting or adding a Transform component
    initialize() {
        _transform = owner.get(Transform)
        if(_transform == null) {
            _transform = owner.addComponent(Transform)
        }
    }

    /// Renders the shape at the entity's position
    render() {
        _shape.render(
            _transform.position,
            1.0,
            _transform.rotation,            
            _mulColor,
            _addColor)
    }

    /// Gets the rendering layer
    layer { _layer }
    /// Sets the rendering layer
    layer=(value) { _layer = value }
    /// Comparison operator for sorting by layer
    <(other) { _layer < other.layer }
    /// Sets the additive color
    addColor=(color) { _addColor = color }
    /// Sets the multiply color
    mulColor=(color) { _mulColor = color }
}

/// Component for rendering text using a custom font
class FontRenderer is ShapeRenderer {

    /// Creates a FontRenderer with the given font, text, and size
    construct new(font, text, fontSize) {
        _font = font
        _text = text
        _fontSize = fontSize
    }

    /// Initializes by getting or adding a Transform component
    initialize() {
        _transform = owner.get(Transform)
        if(_transform == null) {
            _transform = owner.addComponent(Transform)
        }
    }

    /// Renders each character of the text
    render() {
        var position = _transform.position
        var x = position.x
        var y = position.y
        for(i in 0..._text.count) {
            var c = _text[i]
            var glyph = _font.glyphs[c]
            if(glyph) {
                var pos = Vec2.new(x, y)
                var shape = glyph.shape
                shape.render(pos, 20.0, 0)
                x = x + glyph.advance * 20
            }
        }

        /*
        var position = _transform.position
        var scale = _transform.scale
        var rotation = _transform.rotation
        var x = position.x
        var y = position.y
        for(i in 0..._text.length) {
            var c = _text[i]
            var glyph = _font.glyphs[c]
            if(glyph) {
                var pos = Vec2.new(x + glyph.offset.x, y + glyph.offset.y)
                pos = pos.rotated(rotation)
                pos = pos * scale
                Render.drawTexture(glyph.texture, pos, scale, rotation)
                x = x + glyph.advance
            }
        }
        */
    }

    /// Gets the font
    font { _font }
    /// Gets the text string
    text { _text }
}

/// Utility class for shape rendering and generation
class Shapes {
    /// Render all shapes in the scene
    static render() {        
        var entities = Entity.entities
        var shapes = []
        for (entity in entities) {
            var shape = entity.get(ShapeRenderer)
            if(shape) shapes.add(shape)
        }

        shapes.sort()
        for (shape in shapes) {
            shape.render()
        }
    }

    /// Renders text at the given position with the specified font and size
    static renderText(text, font, position, size) {
        var x = position.x
        var y = position.y
        for(i in 0...text.count) {
            var c = text[i]
            var glyph = font.glyphs[c]
            if(glyph) {
                var pos = Vec2.new(x, y)
                var shape = glyph.shape
                shape.render(pos, size, 0)
                x = x + glyph.advance * size
            }
        }
    }

    /// Filled shapes ////////////////////////////////////////////////

    /// Create a quad shape
    static quad(p0, p1, p2, p3, color) {
        var shape = Shape.new()
        shape.vertex(p0, color)
        shape.vertex(p1, color)
        shape.vertex(p2, color)
        shape.vertex(p2, color)
        shape.vertex(p3, color)
        shape.vertex(p0, color)
        return shape
    }

    /// Create a new disk shape (or any regular polygon in fact)
    static disk(center, radius, segments, color) {
        var shape = Shape.new()
        var angle = 0
        var step = 2 * Math.pi / segments
        for(i in 0...segments) {
            var x = angle.cos * radius
            var y = angle.sin * radius
            shape.vertex(center, color)
            shape.vertex(Vec2.new(x, y) + center, color)
            angle = angle + step
            x = angle.cos * radius
            y = angle.sin * radius            
            shape.vertex(Vec2.new(x, y) + center, color)
        }
        return shape
    }

    /// Create a new rectangle shape
    static rectangle(p0, p1, color) {
        var shape = Shape.new()
        var p2 = Vec2.new(p1.x, p0.y)
        var p3 = Vec2.new(p0.x, p1.y)
        return Shapes.quad(p0, p2, p1, p3, color)
    }

    /// Fills a convex polygon with the given color
    static fill(vertices, color) {
        /// Find the center of the polygon
        var center = Vec2.new(0, 0)
        for(vertex in vertices) {
            center = center + vertex
        }
        center = center * 1.0 / vertices.count

        /// Create the shape
        var shape = Shape.new()
        for(i in 0...vertices.count) {
            shape.vertex(center, color)
            shape.vertex(vertices[i], color)
            shape.vertex(vertices[(i + 1) % vertices.count], color)
        }
        return shape
    }

    /// Outlined shapes //////////////////////////////////////////////

    /// Creates a line shape with the given thickness
    static line(p0, p1, thickness, color) {
        var shape = Shape.new()
        var normal = (p1 - p0).normal
        var offset = normal * thickness
        var p2 = p0 + offset
        var p3 = p1 + offset
        var p4 = p1 - offset
        var p5 = p0 - offset
        Shapes.quad(p2, p3, p4, p5, color)
        return shape
    }

    /// Draws a polyline (stroke) through the given points with the specified thickness
    static stroke(points, thickness, color) {

        // Create the inner and outer points
        var inner = []
        var outer = []
        for(i in 0...points.count) {
            /// Use three points of the line to  average the normal
            var p_prev = points[(i - 1) % points.count]
            var p_curr = points[i]
            var p_next = points[(i + 1) % points.count]
            var normal = ((p_curr - p_prev).normal + (p_next - p_curr).normal) * 0.5
            var adjThickness = thickness / normal.dot(normal)
            var offset = normal.perp * adjThickness
            var p0 = p_curr + offset
            var p1 = p_curr - offset
            inner.add(p0)
            outer.add(p1)
        }        

        // Connect the points in the inner and outer lists
        var shape = Shape.new()
        for(i in 0...inner.count) {
            var p0 = inner[i]
            var p1 = outer[i]
            var p2 = outer[(i + 1) % outer.count]
            var p3 = inner[(i + 1) % inner.count]
            shape.vertex(p0, color)
            shape.vertex(p1, color)
            shape.vertex(p2, color)
            shape.vertex(p2, color)
            shape.vertex(p3, color)
            shape.vertex(p0, color)            
        }
        return shape
    }

    /// Point generation ////////////////////////////////////////////

    /// Generates points for a rounded polygon
    static polygon( center, radius, sides,
                    rounding, segments) {
        //  Create the polygon points
        var points = []
        var r = radius - rounding * 2.sqrt
        var step = 2 * Math.pi / sides

        for(i in 0...sides) {
            var angle = i * step
            var cx = angle.cos * r + center.x
            var cy = angle.sin * r + center.y

            var a = angle - step * 0.5
            var da = step / segments
            for(j in 0..segments) {
                var x = cx + a.cos * rounding
                var y = cy + a.sin * rounding
                var p = Vec2.new(x, y)
                points.add(p)
                a = a + da
            }
        }

        return points
    }
}
