#!/usr/bin/env python

import cairo
import random
import colorsys
import math

class vec2:
    def __init__(self, x:float, y:float) -> None:
        self.x = x
        self.y = y

    def __add__(self, other):
        return vec2(self.x + other.x, self.y + other.y)
    
    def __sub__(self, other):
        return vec2(self.x - other.x, self.y - other.y)
    
    def __mul__(self, value:float):
        return vec2(self.x * value, self.y * value)
    
    def magnitude(self) -> float:
        return math.sqrt(self.x**2 + self.y**2)
    
    def normalize(self) -> None:
        m = self.magnitude()
        self.x /= m
        self.y /= m
    
    def normalized(self):
        m = self.magnitude()
        return vec2(self.x / m, self.y / m)

    def __str__(self) -> str:
        return "[{},{}]".format(self.x, self.y)

sizes = [17, 30, 30, 17, 12, 10, 7]

class color:
    def __init__(self, r : float, g : float,  b : float):
        self.r = r
        self.g = g
        self.b = b

    def __add__(self, other) :
        return color(self.r + other.r,
                     self.g + other.g,
                     self.b + other.b)

def next_power_of_2(x):  
    return 1 if x == 0 else 2**(x - 1).bit_length()

def get_color(num):
      hue = float(num) / 8
      (h, s, v) = (hue, 0.4, 0.9)
      (r, g, b) = colorsys.hsv_to_rgb(h, s, v)
      return (r, g, b)

def parts(radius, clr):
    number = len(sizes) 
    next_p2 = next_power_of_2(radius * 2 + 1)
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * number, next_p2)
    colorCtx = cairo.Context(colorSurf)
    colorCtx.translate(next_p2 * 0.5, next_p2 * 0.5)
    for i in range(0, number):
        x = i * next_p2 + next_p2 * 0.5
        y = next_p2 * 0.5
        r = sizes[i] + 3
        colorCtx.arc(0, 0, r, 0.0, math.pi * 2.0)
        colorCtx.set_source_rgb(clr.r, clr.g, clr.b)
        colorCtx.fill()
        clr = clr + color(0.07, 0.07, 0.07)
        colorCtx.set_line_width(5)
        colorCtx.move_to(-r * 0.7, 0)
        colorCtx.line_to(r * 0.7, 0)
        colorCtx.set_source_rgb(clr.r, clr.g, clr.b)
        colorCtx.stroke()
        colorCtx.translate(next_p2, 0.0)
        
    colorSurf.write_to_png("samples/gameplay/assets/images/parts.png")

def arrow(radius):
    number = 5 
    next_p2 = next_power_of_2(int(radius * 8 + 1))
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * number, next_p2)
    colorCtx = cairo.Context(colorSurf)
    for i in range(0, number):
        m = cairo.Matrix()
        m.translate((i+0.5) * next_p2, next_p2 * 0.5)
        m.scale(1 + i * 0.4, 1.0 - i * 0.1)
        colorCtx.set_matrix(m)
        colorCtx.arc(0, 0, radius, 0.0, math.pi * 2.0)
        colorCtx.set_source_rgb(1, 1, 1)
        colorCtx.fill()
    colorSurf.write_to_png("samples/gameplay/assets/images/arrow.png")


def aim(radius):
    number = 8
    next_p2 = next_power_of_2(int(radius * 2 + 1))
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * number, next_p2)
    colorCtx = cairo.Context(colorSurf)   
    colorCtx.translate(next_p2 * 1.5, next_p2 * 0.5)
    for i in range(0, number):            
        a =  (number - i) / number
        a *= math.pi / 6.0
        colorCtx.set_source_rgba(1, 1, 1, 0.5)
        colorCtx.move_to(0, 0)
        colorCtx.line_to( math.cos(a) * radius,  math.sin(a) * radius)
        colorCtx.stroke()
        colorCtx.move_to(0, 0)
        colorCtx.line_to( math.cos(-a) * radius,  math.sin(-a) * radius)        
        colorCtx.stroke()
        colorCtx.translate(next_p2, 0.0)
    colorSurf.write_to_png("samples/gameplay/assets/images/aim.png")


def warning(radius):
    next_p2 = next_power_of_2(radius * 2 + 1)
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * 2, next_p2)
    colorCtx = cairo.Context(colorSurf)
    (r, g, b) = get_color(8)
    colorCtx.set_source_rgb(r, g, b)
    x = next_p2 * 0.5
    y = next_p2 * 0.5
    colorCtx.arc(x, y, radius, 0.0, math.pi * 2.0)
    colorCtx.stroke()

    colorSurf.write_to_png("samples/gameplay/assets/images/warning.png")


def ellipse(radius : float, r : float, g : float, b : float, sx : float, sy : float, file : str):
    next_p2 = next_power_of_2(int(radius) * int(max(sx, sy)) * 2 + 1)
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2, next_p2)
    colorCtx = cairo.Context(colorSurf)
    x = next_p2 * 0.5    
    y = next_p2 * 0.5
    colorCtx.scale(sx, sy)    
    colorCtx.arc(x / sx, y / sy, radius, 0.0, math.pi * 2.0)
    colorCtx.set_source_rgb(r, g, b)
    colorCtx.fill()    
    colorSurf.write_to_png("samples/gameplay/assets/images/" + file)

def circle(radius : float, r : float, g : float, b : float, file : str):
    ellipse(radius, r, g, b, 1.0, 1.0, file)

def checkerboard(width : int, height : int, size : int, fc : color, sc : color, file : str):
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    colorCtx = cairo.Context(colorSurf)
    tx = int(width / size)
    ty = int(height / size)
    for i in range(0, tx):
        x = i * size
        for j in range(0, ty):            
            y = j * size
            colorCtx.rectangle(x, y, size, size)
            if ((i + j) % 2 == 0):
                colorCtx.set_source_rgb(fc.r, fc.g, fc.b)
            else: 
                colorCtx.set_source_rgb(sc.r, sc.g, sc.b)
            colorCtx.fill()    
    colorSurf.write_to_png("samples/gameplay/assets/images/" + file)

def box(width : int, height : int, r : float, g : float, b : float, file : str):
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    colorCtx = cairo.Context(colorSurf)
    colorCtx.rectangle(0, 0, width, height)
    colorCtx.set_source_rgb(r, g, b)
    colorCtx.fill()  
    colorSurf.write_to_png("samples/gameplay/assets/images/" + file)
     
def healthbar(width : int, size : int, cl : color, file : str):
    height = 100 * size    
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    colorCtx = cairo.Context(colorSurf)
    tx = width / 100.0

    for j in range(1, 101): 
        y = height - j * size
        w = tx * j 
        colorCtx.rectangle(width * 0.5 - w * 0.5, y, w, size)
        colorCtx.set_source_rgb(cl.r, cl.g, cl.b)
        colorCtx.fill()    
    colorSurf.write_to_png("samples/gameplay/assets/images/" + file)

class ExplosionBall:
    def __init__(self, size: float, position: vec2) -> None:
        self.position = position
        self.size = size
    
    def draw(self, ctx: cairo.Context, param: float) -> None :
        p = math.pow(param, 1/4)
        pos = vec2(0,0) * (1.0 - p) + self.position * p
        rad = self.size * p
        ctx.set_source_rgb(1, 1, 1)
        ctx.arc(pos.x, pos.y, rad, 0.0, math.pi * 2)
        ctx.set_operator(cairo.Operator.OVER)
        ctx.fill()
        ctx.set_source_rgba(1, 1, 1, 1)
        ctx.set_operator(cairo.Operator.XOR)
        s_p = math.pow(param, 2)
        s_rad = rad * s_p
        s_pos = pos - pos.normalized() * (rad - s_rad)
        ctx.arc(s_pos.x, s_pos.y, s_rad, 0.0, math.pi * 2)
        ctx.fill()

def explosion(size : int, frames: int, file : str) :
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, size * frames, size)
    colorCtx = cairo.Context(colorSurf)
    colorCtx.translate(size * 0.5, size * 0.5)
    ebs = [
        ExplosionBall(size * 0.25, vec2(size * 0.2, size * 0.2)),
        ExplosionBall(size * 0.2, vec2(size * 0.3, -size * 0.24)),
        ExplosionBall(size * 0.3, vec2(size * -0.1, -size * 0.1)),
        ExplosionBall(size * 0.15, vec2(size * -0.1, -size * -0.1)),
    ]

    for i in range(1, frames + 1):
        for e in ebs:
            e.draw(colorCtx, i / frames)
        colorCtx.translate(size, 0)
    colorSurf.write_to_png("samples/gameplay/assets/images/" + file)


def player() : 
    (r,g,b) = (1, 1, 1)
    w = 64
    h = 64
    s = 24
    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, w, h)
    ctx = cairo.Context(surf)
    ctx.translate(w * 0.5, h * 0.5)
    dt = math.pi * 2 / 12
    t = -dt * 0.5
    ctx.new_path()
    for i in range(0, 6) :
        r = s
        if i < 2:
            r = s * 1.2
        x = math.cos(t) * r
        y = math.sin(t) * r
        if i == 0:
            ctx.move_to(x,y)
        else:   
            ctx.line_to(x,y)
        if i % 2 == 0:
            t += dt
        else:   
            t += 3.0 * dt
    ctx.close_path() 
    ctx.set_source_rgb(r, g, b)
    ctx.fill()    
    surf.write_to_png("samples/gameplay/assets/images/player.png")

player()
aim(60)

(r, g, b) = get_color(3)
ellipse(3, r, g, b, 1.0, 3.0, "missile.png")

'''
arrow(3.5)
circle(15, 0.35, 0.35, 0.35, "foot.png")
(r, g, b) = get_color(3)
circle(18, r, g, b, "shield.png")
(r, g, b) = get_color(1)
circle(10, r, g, b, "missile.png")
(r, g, b) = get_color(8)
warning(35)

(r, g, b) = get_color(8)
circle(15, r, g, b, "hitbox15.png")
circle(20, r, g, b, "hitbox20.png")
circle(25, r, g, b, "hitbox25.png")
circle(30, r, g, b, "hitbox30.png")

#dark = color(0.0462, 0.0700, 0.0525)
#light = color(0.0372, 0.0600, 0.0433)
dark = color(0.1, 0.1, 0.1)
light = color(0.13, 0.13, 0.13)
checkerboard(640, 360, 40, dark, light, "background.png")

explosion(64, 16, "explosion.png")

part_color = color(0.55, 0.55, 0.55)
parts(36, part_color)

(r, g, b) = get_color(8)
ellipse(3, r, g, b, 1.0, 3.0, "missile.png")

box(8, 1024, r, g, b, "beam_1.png")
box(1, 1024, r, g, b, "beam_0.png")


parts(8, "1")
parts(14, "2")
parts(20, "3")

circle(12, 0.776, 0.277, 0.990, "ships/core.png")


circle(2, 0.990, 0.622, 0.277, "ships/jump_target.png")
ellipse(2.5, 0.990, 0.622, 0.277, 1.0, 3.0, "projectiles/pl_cannon.png")

(r, g, b) = get_color(0)
circle(4, r, g, b, "projectiles/cannon.png")

(r, g, b) = get_color(1)
box(16, 360, r, g, b, "projectiles/beam_3.png")
box(12, 360, r, g, b, "projectiles/beam_2.png")
box(8, 360, r, g, b, "projectiles/beam_1.png")
box(1, 360, r, g, b, "projectiles/beam_0.png")

mid = color(0.15, 0.15, 0.15)
sidewalls(640, 360, 20, mid,  "background/side.png")

white = color(1.0, 1.0, 0.9)
healthbar(400, 4, white, "ui/healthbar_boss.png")

'''