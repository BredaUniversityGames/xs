import "xs" for Input, Render
import "random" for Random
import "vector" for Vector, ColorRGBA, Base

class Tag {
    static None { 0 }
    static ShipPlayer { 1 }
    static ShipEnemy { 2 }
    static BulletPlayer { 3 }
    static BulletEnemy { 4 }
}

class GameObject {
    construct new() {
        _deleted = false
        _tag = 0
        Game.addObject(this)
    }
    update(dt) { }
    render() { }
    deleted { _deleted }
    delete() { _deleted = true }
    tag { _tag }
    tag=(t) { _tag = t }
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
        Render.rect(_position.x, _position.y, _size.x, _size.y)
    }
}

class Ship  is MovingObject {
    construct new() {
        var x = Data.getNumber("Width", Data.system) * -0.25
        var pos = Vector.new(x, 0.0)
        var vel = Vector.new(0.0, 0.0)
        var size = Vector.new(35.0, 15.0)
        super(pos, size, vel)
        _rect = Rect.positionSize(Vector.new(0,0), size)
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
        var h = Data.getNumber("Height", Data.system) * 0.5
        var w = Data.getNumber("Width", Data.system) * 0.5
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
        Render.setColor(0xFBAF40FF)
        Render.disk(position.x, position.y, 5, 24)
        Render.setColor(0xD07D04FF)        
        Render.begin(Render.triangles)
            Render.vertex(position.x, position.y)
            Render.vertex(position.x + 6, position.y + 6)
            Render.vertex(position.x - 17, position.y + (12 - open) - tilt)
            Render.vertex(position.x, position.y)
            Render.vertex(position.x + 6, position.y - 6)
            Render.vertex(position.x - 17, position.y - (12 - open) - tilt)
        Render.end()
    }
}


class Enemy  is MovingObject {
    construct new() {
        var x = Data.getNumber("Width", Data.system) * 0.25
        var pos = Vector.new(x, 0.0)
        var vel = Vector.new(0.0, 0.0)
        var size = Vector.new(35.0, 15.0)
        super(pos, size, vel)
        _rect = Rect.positionSize(Vector.new(0,0), size)
        _shootTime = 0.0
    }

    update(dt) {
        var toPos = Game.playerShip.position
        var dir = toPos - position
        dir.x = 0.0
        dir.normal
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
        Render.setColor(0xC859FFFF)
        Render.disk(position.x, position.y, 8, 24)
        Render.setColor(0xF477FFFF)        
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
        if (position.x.abs > Data.getNumber("Width", Data.system)) {            
            delete()
        }
    }

    render() {
        if(velocity.x > 0) {
            Render.setColor(0xFBAF40FF)
            var sx = size.x * 0.5
            var sy = size.y * 0.5
            Render.rect(position.x - sx, position.y - sy, position.x + sx, position.y + sy)
        } else {
            Render.setColor(0xF477FFFF)
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
        dir.normal
        velocity = dir * 1.0
        move(dt)
    }

    render() {
        Render.setColor(0xF477FFFF)
        Render.disk(position.x, position.y, size.x, 4)
    }
}

class Rect {
    construct fromTo(from, to) {
        _from = from
        _to = to
    }

    construct positionSize(position, size) {
        _from = Vector.new(position.x - size.x * 0.5, position.y - size.y * 0.5)
        _to = Vector.new(position.x + size.x * 0.5, position.y + size.y * 0.5)
    }

    render(x, y) {        
        Render.rect(_from.x + x, _from.y + y, _to.x + x, _to.y + y)        
    }

}

class Building is GameObject {
    static init() {
        __random = Random.new()
        __fromHeight = [0.1, 0.2, 0.4, 0.75]
        __toHeight = [0.15, 0.3, 0.5, 0.85]
        __fromWidth = [30, 30, 25, 20]
        __toWidth = [80, 40, 35, 25]
        __colors = [0x6C5A9DFF, 0x4DAAEFFF, 0x7DC4F7FF, 0xA9E0FFFF]
    }

    construct new(layer) {        
        super()
        var fromH = __fromHeight[layer] * Data.getNumber("Height", Data.system)
        var toH = __toHeight[layer] * Data.getNumber("Height", Data.system)
        var fromW = __fromWidth[layer]
        var toW = __toWidth[layer]

        _width = 120        
        _height = __random.int(fromH, toH)
        _scroll = __random.int(-_width, Data.getNumber("Width", Data.system) + _width)

        _color = __colors[layer]
        _layer = layer * 0.25        
        _scrollSpeed = (1.0 - _layer) * 60 + 40            
        _rects = []

        var x = -100
        var y = 32
        var dx = __random.float(fromW, toW)
        var dy = 15 
        var n = __random.int(3, 3 + layer)
        for(i in 1..n) {
            var from = Vector.new(x, y)
            y = y + __random.float(1, 4) * dy
            var to = Vector.new(x + dx, y)            
            var r = Rect.fromTo(from, to)            
            _rects.add(r)
            y = y + 2
        }
        var from = Vector.new(x + 2, 30)
        var to = Vector.new(x + dx - 2, y)
        var r = Rect.fromTo( from, to)        
        _rects.add(r)      
    }

    update(dt) {
        _scroll = _scroll - _scrollSpeed * dt
        if(_scroll < -_width - Data.getNumber("Width", Data.system) * 0.5) {
            _scroll = _scroll + Data.getNumber("Width", Data.system) + 2 * _width
        }        
    }

    render() {
        Render.setColor(_color)
        for(r in _rects) {
            r.render(_scroll.round, -Data.getNumber("Height", Data.system) * 0.5)
        }
    }

    layer { _layer }
}

class Sprawl is Building {

     static init() {
        __random = Random.new()
        __fromHeight = [0.1, 0.2, 0.4, 0.75]
        __toHeight = [0.15, 0.3, 0.5, 0.85]
        __fromWidth = [30, 30, 25, 20]
        __toWidth = [80, 40, 35, 25]
        __colors = [0x6C5A9DFF, 0x4DAAEFFF, 0x7DC4F7FF, 0xA9E0FFFF]
    }

    construct new(layer, offset) {        
        super()
        var fromH = __fromHeight[layer] * Data.getNumber("Height", Data.system)
        var toH = __toHeight[layer] * Data.getNumber("Height", Data.system)
        var fromW = __fromWidth[layer]
        var toW = __toWidth[layer]

        _width = 120
        _height = __random.int(fromH, toH)
        _scroll = -(Data.getNumber("Width", Data.system) * 0.5) + (Data.getNumber("Width", Data.system) * offset)

        _color = __colors[layer]
        _layer = layer * 0.25        
        _scrollSpeed = (1.0 - _layer) * 60 + 40    
        
        _rects = []
        
        var x = 0
        var dx = 6
        var h = __random.float(30, 50)
        for(i in 1..107) {
            var from = Vector.new(x, 30.0)
            x = x + dx
            var to = Vector.new(x, h)
            h = __random.float(40, 55.0) + layer * 10
            var r = Rect.fromTo(from, to)
            _rects.add(r)
        }    
    }

    update(dt) {

        _scroll = _scroll - _scrollSpeed * dt
        if(_scroll < Data.getNumber("Width", Data.system) * -1.5) {
            _scroll = _scroll + Data.getNumber("Width", Data.system) * 2 
        }        
    }

    render() {
        Render.setColor(_color)
        for(r in _rects) {
            r.render(_scroll.round, -Data.getNumber("Height", Data.system) * 0.5)
        }
    }
}


class Game {

    static init() {
        Building.init()
        Sprawl.init()

        Data.getNumber("Width", Data.system) = 640
        Data.getNumber("Height", Data.system) = 360
        Configuration.multiplier = 1
        Configuration.title = "City"

        __objects = []
        __addQueue = []

        for(layer in 3..0) {
            var n = 5 + 3 * layer
            for (x in 0..n) {
                if(x < 2) {
                    var s = Sprawl.new(layer, x)
                } else {
                    var b = Building.new(layer)
                }                
            }
        }

        var e = Enemy.new()
        __ship = Ship.new()
    }    
    
    static update(dt) {
        Render.setColor(1, 1, 1)
        Render.rect(-Data.getNumber("Width", Data.system), -Data.getNumber("Height", Data.system), Data.getNumber("Width", Data.system), Data.getNumber("Height", Data.system))        

        for(a in __addQueue) {
            __objects.add(a)
        }
        __addQueue.clear()

        /*
        for (o1 in __objects) {
            for (o2 in __objects) {
                if(o1.Rocket) {
                    System.print("Layer!")
                }
                if(o1 != o2) {
                    var l = o1.layer()
                    if(l != null) {
                        System.print("Layer!")
                    }
                }
            }
        }
        */

        for (o in __objects) {
            o.update(dt)            
        }

        for (o in __objects) {
            o.render()
        }

        Render.setColor(0x6C5A9DFF)
        Render.shapeText("SCORE", -200.0, 170.0, 1.0)

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
