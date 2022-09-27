from re import A
import numpy as np
import math
from xspy import init, shutdown, device, input, render, config, fileio
from datetime import datetime, timedelta
from abc import ABC, abstractmethod

class Vector2D:
    def __init__(self, x=0., y =0.) -> None:
        self.x = x
        self.y = y
    
    def __add__(self, o):
        x = self.x + o.x 
        y = self.y + o.y 
        return Vertex(x, y)

    def __sub__(self, o):
        x = self.x - o.x 
        y = self.y - o.y 
        return Vertex(x, y)  

    def is_zero(self,) -> bool:
        return self.x == self.y == 0

    def modulo(self,) -> float:
        return math.sqrt(self.x**2 + self.y**2)

    def normalize(self,):
        if not self.is_zero():
            mod = self.modulo()
            return Vector2D(self.x/mod, self.y/mod)
        else:
            return Vector2D()

    def dot(self, other):
        return self.x * other.x + self.y * other.y
    
    def numpy(self,):
        return np.array(self.x, self.y)


class Matrix2D:
    def identity():
        return [[1,0,0],
                [0,1,0],
                [0,0,1]]
    
    def translate(x, y):
        return [[1,0,0],
                [0,1,0],
                [x,y,1]]

    def scale(x, y):
        return [[x,0,0],
                [0,y,0],
                [0,0,1]]

    # theta in radians
    def rotate_origin(theta):
        c_t = math.cos(theta)
        s_t = math.sin(theta)
        return [[c_t,   s_t,  0],
                [-s_t,  c_t,  0],
                [0,     0,    1]]

    def scale(x, y):
        return [[x,0,0],
                [0,y,0],
                [0,0,1]]     

    # theta in radians
    def shearx(t):
        return [[1,t,0],
                [0,1,0],
                [0,0,1]]   
    
    # theta in radians
    def sheary(t):
        return [[1,0,0],
                [t,1,0],
                [0,0,1]]   

    # theta in radians
    def shearx_angle(theta):
        t_t = math.tan(theta)
        return Matrix2D.shearx(t_t)    

    # theta in radians
    def sheary_angle(theta):
        t_t = math.tan(theta)
        return Matrix2D.sheary(t_t) 

    def reflecto():
        return [[-1,0,0],
            [0,-1,0],
            [0,0,1]] 

    def reflectx():
        return [[1,0,0],
            [0,-1,0],
            [0,0,1]] 

    def reflecty():
        return [[-1,0,0],
            [0,1,0],
            [0,0,1]] 

    def model(translation, theta, scale):
        t_x,t_y = translation
        s_x, s_y = scale
        T = Matrix2D.translate(t_x,t_y)
        R = Matrix2D.rotate_origin(theta)
        S = Matrix2D.scale(s_x, s_y)
        return np.matmul(np.matmul(S,R), T)

    def batch_mult(batch, matrix):
        return [np.matmul(matrix, b) for b in batch]



keyW = 87
keyA = 65
keyS = 83
keyD = 68

class Player:
    def __init__(self, position=np.array([0., 0.]), speed=50., side_len=64):
        self.position = position
        self.speed = speed
        self.img = render.load_image('games\jump-core\images\ships\Purple-3.png')
        self.sprite = render.create_sprite(self.img, 0., 0., 1., 1.)
        self.color_add = render.color(0, 0, 0, 0)
        self.color_mul = render.color(255, 255, 255, 255)
        self.window_size = [config.get_width(), config.get_height()]
        self.angle = 0.
        #print(self.window_transform)

    def update(self, dt):
        dir = np.zeros((2,), dtype=np.float32)
        if input.get_key(keyW):
            dir[1] += self.speed*dt
        if input.get_key(keyA):
            dir[0] -= self.speed*dt
        if input.get_key(keyS):
            dir[1] -= self.speed*dt
        if input.get_key(keyD):
            dir[0] += self.speed*dt
        self.position += dir

        mouse_pos = np.array([input.get_mouse_x(), input.get_mouse_y()])
        dir = mouse_pos - self.position
        
        dir = Vector2D(dir[0], dir[1]).normalize()
        # angle from 2D vector
        self.angle = math.atan2(dir.y, dir.x)
    
    def render(self):
        render.render_sprite(
            self.sprite, self.position[0], 
            self.position[1], 
            1., 
            self.angle, 
            self.color_mul, 
            self.color_add, 
            4)

class Vertex:
    def __init__(self, x: float, y: float) -> None:
        self.x = x
        self.y = y

   

class Renderable(ABC):
    @abstractmethod
    def render(self):
        pass


class Triangle(Renderable):
    def __init__(self, a: Vector2D, b: Vector2D, c: Vector2D) -> None:
        self.a = a
        self.b = b
        self.c = c
        self.color = np.array([1., 0., 0., 1.])

    def render(self):
        render.begin(render.primitive.triangles)
        render.set_color(*self.color)
        render.vertex(*self.a.numpy())
        render.vertex(*self.b.numpy())
        render.vertex(*self.c.numpy())
        render.end()

class Rectangle(Renderable):
    def __init__(self, tl: Vector2D, br: Vector2D) -> None:
        self.tl = tl
        self.br = br
        size = tl - br
        self.tr = tl
        self.tr.x += size.x
        self.bl = tl
        self.bl.y -= size.y
        self.color = np.array([1., 0., 0., 1.])


    def render(self):
        triangles = [
            [self.tl, self.tr, self.br],
            [self.tl, self.br, self.bl]
        ]
        render.begin(render.primitive.triangles)
        render.set_color(*self.color)
        for t in triangles:
            render.vertex(*t[0].numpy())
            render.vertex(*t[1].numpy())
            render.vertex(*t[2].numpy())
        render.end()

class Arrow(Renderable):
    def __init__(self, start: Vector2D, end: Vector2D, ) -> None:
        arrow_width = 10
        line_width = 4
        

    def render(self):
        pass


if __name__ == "__main__":
    init()
    fileio.add_wildcard("[games]", "../games")
    
    prev_time = datetime.now()
    player = Player()
    while not device.should_close():
        curr_time = datetime.now()
        elapsed = curr_time - prev_time
        prev_time = curr_time
        dt = elapsed/timedelta(seconds=1)
        
        device.poll_events()
        input.update(dt)
        player.update(dt)
        render.clear()
        player.render()
        render.render()
        device.swap_buffers()

    shutdown()