#!/usr/bin/env python

import cairo
import random
import colorsys
import math
import datetime
from perlin_noise import PerlinNoise
import matplotlib.colors as colors

def roundrect(context, x, y, width, height, r):
    context.arc(x+r, y+r, r,
                math.pi, 3*math.pi/2)
    context.arc(x+width-r, y+r, r,
                3*math.pi/2, 0)
    context.arc(x+width-r, y+height-r,
                r, 0, math.pi/2)
    context.arc(x+r, y+height-r, r,
                math.pi/2, math.pi)
    context.close_path()

def roundrect2(context, x, y, width, height, r):
    context.rectangle(x + r, y, width - 2 * r, height)
    context.rectangle(x, y + r, width, height - 2 * r)
    context.fill() 

def starified(value: int, spread: int, divisions: int) :
    l = list()
    f = value - spread / 2
    #t = value + spread
    d = spread / (divisions)
    r = int(d / 2 * 0.4)
    for i in range(0, divisions) :
        v = i  * d + d / 2 + random.randint(-r, r)
        l.append(int(f + v))
    return l

image_width, image_height = 640, 360
seed = 1002797
random.seed()
base = random.random() * 0.2 + 0.5
noise = PerlinNoise(octaves=8, seed=seed)
fc = (46.0 / 255.0, 136.0 / 255.0, 241.0 / 255.0)

def lerp(x, y, t : float) :
    return x * (1.0 - t) + y * t

def background(ctx : cairo.Context):
    ctx.set_source_rgb(1,1,1)
    ctx.rectangle(0, 0, image_width, image_height)
    ctx.fill()

def sprawl(ctx : cairo.Context, from_x : int, to_x : int, from_y : int, to_y : int, color):
    ctx.move_to(0, 0)
    x = from_x
    h = random.randint(from_y, to_y)
    ctx.set_source_rgb(color[0], color[1], color[2])
    while x < to_x :
        dx = random.randint(2, 4) 
        nh = h + random.randint(-4, 4)
        if nh > to_y or nh < from_y:
            nh = 2  *h - nh
            dx += dx
        f = min(math.sin( (x - from_x) / (to_x - from_x)  * 3.14), 0.2) * 5
        ctx.rectangle(x, 0, dx, h * f)
        x += dx
        h = nh        
    ctx.fill()

def building(ctx : cairo.Context, x : int, wd : float, hg : float, color):
    ctx.set_source_rgb(color[0], color[1], color[2])
    #ctx.rectangle(int(x - wd * 0.5), 0, wd, hg)
    roundrect2(ctx, int(x - wd * 0.5), 0, wd, hg, 2)
    ctx.fill()

"""
def buildings(ctx: cairo.Context):
    for i in range(0, 20):
        x = i * image_width / 20
        x += random.randrange(-10, 10) 
        #x = random.random() * width
        w = random.randrange(25, 45)
        h = random.randrange(90, 260)
        d = random.random() * 0.6 + 0.2
        building(ctx, x, w, h, d)
"""

def main():
    

    surf = cairo.ImageSurface(cairo.FORMAT_ARGB32, image_width, image_height)
    ctx = cairo.Context(surf)

    background(ctx)

    mtx = cairo.Matrix(1, 0, 0, -1, 0, image_height)
    ctx.set_matrix(mtx)

    c = colors.hex2color('#06AFFA')
    sprawl(ctx, 200, 400, 10, 22, c)
    for i in range(0, 3) :
        building(ctx, random.randint(200, 400), random.randint(20, 30), 120, c)


    c = colors.hex2color('#2E88F1')
    sprawl(ctx, 100, 300,  4, 16, c)
    for x in starified(200, 200, 3) :
        building(   ctx,
                    x,
                    random.randint(20, 22),
                    random.randint(35, 62),
                    c)

    dt = datetime.datetime.now()
    ts = dt.timestamp()

    surf.write_to_png("games/jump-core/images/city/city-current.png")
    surf.write_to_png("games/jump-core/images/city/city-" +  str(int(ts)) +".png")
    print("Done")

main()
