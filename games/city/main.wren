import "xs" for Configuration, Input, Render
import "random" for Random
import "vector" for Vector, ColorRGBA, Base


class GameObject {
    construct new() {
        _deleted = false
        Game.addObject(this)
    }
    update(dt) { }
    render() { }
    deleted { _deleted }
    delete() { _deleted = true }
}

class MovingObject is GameObject {
    construct new(position, size, velocity) {
        super()
        _position = position
        _size = size
        _velocity = velocity
    }

    position { _position }
    size { _size }
    velocity { _velocity }

    position=(p) { _position = p }
    size=(s) { _size = s }
    velocity=(v) { _velocity = v }

    move(dt) {
        _position = _position + _velocity * dt
    }

    collide(other) {
        
    }

    debugRender(color) {
        Render.setColor(color.r, color.g, color.b)
        Render.rect(_position.x, _position.y, _size.x, _size.y, 0.0)
    }
}

class Ship  is MovingObject {
    construct new() {
        var x = Configuration.width * -0.25
        var pos = Vector.new(x, 0.0)
        var vel = Vector.new(0.0, 0.0)
        var size = Vector.new(35.0, 15.0)
        super(pos, size, vel)
        _rect = Rect.positionSize(Vector.new(0,0), size, 0.0)
        _shootTime = 0.0
    }

    update(dt) {
        var vel = Vector.new(Input.getAxis(0), -Input.getAxis(1))
        // var vel = Vector.new(Input.getKey(Input.keyRight) - Input.getKey(Input.keyLeft), 0)

        if(Input.getKey(Input.keyLeft)) {
            vel.x = vel.x - 1
        }
        if(Input.getKey(Input.keyRight)) {
            vel.x = vel.y + 1
        }

         if(Input.getKey(Input.keyUp)) {
            vel.y = vel.y + 1
        }
        if(Input.getKey(Input.keyDown)) {
            vel.y = vel.y - 1
        }

        if(vel.magnitude > 0.10) {            
            vel = vel * 500.0
        } else {
            vel = vel * 0.0
        }

        velocity = vel
        move(dt)

        // Keep in bounds
        var h = Configuration.height * 0.5
        var w = Configuration.width * 0.5
        if (position.x < -w) {
            position.x = -w
        } else if (position.x > w) {
            position.x = w
        }
        if (position.y < -h) {
            position.y = -h
        } else if (position.y > h) {
            position.y = h
        }

        _shootTime = _shootTime + dt
        if(Input.getButton(0) == true && _shootTime > 0.08) {
            Game.createBullet(this)
            _shootTime = 0.0
        }
    }

    render() {
        var open = velocity.x * 0.02
        var tilt = velocity.y * 0.02
        Render.setColor("FBAF40FF")
        Render.disk(position.x, position.y, 8, 24)
        Render.setColor("D07D04FF")        
        Render.begin(Render.triangles)
            Render.vertex(position.x, position.y)
            Render.vertex(position.x + 10, position.y + 10)
            Render.vertex(position.x - 30, position.y + (20 - open) - tilt)
            Render.vertex(position.x, position.y)
            Render.vertex(position.x + 10, position.y - 10)
            Render.vertex(position.x - 30, position.y - (20 - open) - tilt)
        Render.end()
    }
}


class Enemy  is MovingObject {
    construct new() {
        var x = Configuration.width * 0.25
        var pos = Vector.new(x, 0.0)
        var vel = Vector.new(0.0, 0.0)
        var size = Vector.new(35.0, 15.0)
        super(pos, size, vel)
        _rect = Rect.positionSize(Vector.new(0,0), size, 0.0)
        _shootTime = 0.0
    }

    update(dt) {
        var toPos = Game.playerShip.position
        var dir = toPos - position
        dir.x = 0.0
        dir.normalise
        velocity = dir * 1.5
        move(dt)
        
        _shootTime = _shootTime + dt
        if(_shootTime > 0.3) {
            Game.createBullet(this)
            _shootTime = 0.0
        }
    }

    render() {
        var open = velocity.x * 0.02
        var tilt = velocity.y * 0.02
        Render.setColor("C859FFFF")
        Render.disk(position.x, position.y, 8, 24)
        Render.setColor("F477FFFF")        
        Render.begin(Render.triangles)
            Render.vertex(position.x, position.y)
            Render.vertex(position.x - 10, position.y + 10)
            Render.vertex(position.x + 30, position.y + (20 - open) - tilt)
            Render.vertex(position.x, position.y)
            Render.vertex(position.x - 10, position.y - 10)
            Render.vertex(position.x + 30, position.y - (20 - open) - tilt)
        Render.end()
    }
}

class Bullet is MovingObject {
    construct new(owner) {
        var vel
        var size
        if(owner == Game.playerShip){
            vel = Vector.new(900.0, 0.0)
            size = Vector.new(10.0, 10.0)
        } else {
            vel = Vector.new(-300.0, 0.0)
            size = Vector.new(12.0, 12.0)
        }
        super(owner.position, size, vel)
    }

    update(dt) {
        move(dt)
        if (position.x.abs > Configuration.width) {            
            delete()
        }
    }

    render() {
        if(velocity.x > 0) {
            Render.setColor("FBAF40FF")
            var sx = size.x * 0.5
            var sy = size.y * 0.5
            Render.rect(position.x - sx, position.y - sy, position.x + sx, position.y + sy)
        } else {
            Render.setColor("F477FFFF")
            Render.disk(position.x, position.y, size.x, 4)
        }
    }
}

class Rocket is MovingObject {
    construct new(position) {
        var vel = Vector.new(0.0, 0.0)
        var size = Vector.new(8.0, 8.0)
        super(position, size, vel)
    }

    update(dt) {
        var toPos = Game.playerShip.position
        var dir = toPos - position
        dir.normalise
        velocity = dir * 1.0
        move(dt)
    }

    render() {
        Render.setColor("F477FFFF")
        Render.disk(position.x, position.y, size.x, 4)
    }
}

class Rect {
    construct fromTo(from, to, rounding) {
        _from = from
        _to = to
        _rounding = rounding
    }

    construct positionSize(position, size, rounding) {
        _from = Vector.new(position.x - size.x * 0.5, position.y - size.y * 0.5)
        _to = Vector.new(position.x + size.x * 0.5, position.y + size.y * 0.5)
        _rounding = rounding
    }

    render(x, y) {
        if(_rounding == 0.0) {
            Render.rect(_from.x + x, _from.y + y, _to.x + x, _to.y + y)
        } else {

        }
    }

}

class Building is GameObject {
    static init() {
        __random = Random.new()
        __fromHeight = [0.1, 0.2, 0.4, 0.75]
        __toHeight = [0.15, 0.3, 0.5, 0.85]
        __colors = ["6C5A9DFF", "4DAAEFFF", "7DC4F7FF", "A9E0FFFF"]
    }

    construct new(layer) {        
        super()
        var fromH = __fromHeight[layer] * Configuration.height
        var toH = __toHeight[layer] * Configuration.height

        _width = 120        
        _height = __random.int(fromH, toH)
        _scroll = __random.int(-_width, Configuration.width + _width)

        _color = __colors[layer]
        _layer = layer * 0.25        
        _scrollSpeed = (1.0 - _layer) * 60 + 40    
        
        _rects = []
        var x = -100
        var dx = 12
        var h = __random.float(fromH, toH)
        for(i in 1..5) {
            var from = Vector.new(x, 0.0)
            x = x + __random.int(1, 3) * dx            
            var to = Vector.new(x, h)
            h = h + __random.float(-6.0, 6.0)
            var r = Rect.fromTo(from, to, 0.0)
            _rects.add(r)
        }
    }

    update(dt) {
        _scroll = _scroll - _scrollSpeed * dt
        if(_scroll < -_width - Configuration.width * 0.5) {
            _scroll = _scroll + Configuration.width + 2 * _width
        }        
    }

    render() {
        Render.setColor(_color)
        for(r in _rects) {
            r.render(_scroll.round, -Configuration.height * 0.5)
        }
    }

    layer { _layer }
}


class Game {

    static init() {
        Building.init()
        Configuration.width = 540
        Configuration.height = 240
        Configuration.multiplier = 3
        Configuration.title = "City"

        __objects = []
        __addQueue = []

        for(layer in 3..0) {
            for (x in 0..5) {
                var b = Building.new(layer)
            }
        }

        var e = Enemy.new()
        __ship = Ship.new()
    }    
    
    static update(dt) {
        Render.setColor(1, 1, 1)
        Render.rect(0, 0, Configuration.width, Configuration.height, 0.0)        

        for(a in __addQueue) {
            __objects.add(a)
        }
        __addQueue.clear()

        for (o in __objects) {
            o.update(dt)            
        }

        for (o in __objects) {
            o.render()
        }

        var i = 0
        while(i < __objects.count) {
            var o = __objects[i]
            if(o.deleted){
                __objects.removeAt(i)
            } else {
                i = i + 1
            }
            
        }
    }

    static render() {
        
    }

    static addObject(object) {
        __addQueue.add(object)
    }

    static createBullet(parent) {
        var b = Bullet.new(parent)
    }

    static createRocket(position) {        
    }

    static playerShip { __ship }
}
