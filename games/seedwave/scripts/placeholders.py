#!/usr/bin/env python

import cairo
import random
import colorsys
import math

letters = ["C", "L", "M", "D", "N", "E", "H", "V"]

class color:
    def __init__(self, r : float, g : float,  b : float):
        self.r = r
        self.g = g
        self.b = b

def next_power_of_2(x):  
    return 1 if x == 0 else 2**(x - 1).bit_length()

def get_color(num):
      hue = float(num) / len(letters)
      (h, s, v) = (hue, 0.4, 0.9)
      (r, g, b) = colorsys.hsv_to_rgb(h, s, v)
      return (r, g, b)

def parts(radius, name):
    number = len(letters) 
    next_p2 = next_power_of_2(radius * 2 + 1)
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * number, next_p2)
    colorCtx = cairo.Context(colorSurf)
    for i in range(0, number):
        x = i * next_p2 + next_p2 * 0.5
        y = next_p2 * 0.5
        colorCtx.arc(x, y, radius, 0.0, math.pi * 2.0)
        (r, g, b) = get_color(i)
        colorCtx.set_source_rgb(r, g, b)
        colorCtx.fill()

        l = letters[i]
        colorCtx.set_font_size(radius * 0.75)
        colorCtx.select_font_face("Mono", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
        colorCtx.set_source_rgb(1, 1, 1)
        (xb, yb, w, h, dx, dy) = colorCtx.text_extents(l)
        #colorCtx.move_to(x - size * 0.3, y + size * 0.3)
        colorCtx.move_to(x - w * 0.4, y + h * 0.5)
        colorCtx.show_text(l)
        colorCtx.fill()

    colorSurf.write_to_png("games/seedwave/assets/images/ships/parts_" + name + ".png")


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
    colorSurf.write_to_png("games/seedwave/assets/images/" + file)

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
    colorSurf.write_to_png("games/seedwave/assets/images/" + file)



def sidewalls(width : int, height : int, size : int, cl : color, file : str):
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    colorCtx = cairo.Context(colorSurf)
    tx = int(width / size)
    ty = int(height / size)
       
    for j in range(0, ty):            
        y = j * size
        colorCtx.rectangle(0, y, size * 2 +  2 * size * random.random(), size)
        colorCtx.set_source_rgb(cl.r + random.random() * 0.02,
                                cl.g + random.random() * 0.02,
                                cl.b + random.random() * 0.02)        
        colorCtx.fill()    

        colorCtx.rectangle(width - size * 2 -  2 * size * random.random(), y, size * 5, size)
        colorCtx.set_source_rgb(cl.r + random.random() * 0.02,
                                cl.g + random.random() * 0.02,
                                cl.b + random.random() * 0.02)        
        colorCtx.fill()    
    colorSurf.write_to_png("games/seedwave/assets/images/" + file)

def box(width : int, height : int, r : float, g : float, b : float, file : str):
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    colorCtx = cairo.Context(colorSurf)
    colorCtx.rectangle(0, 0, width, height)
    colorCtx.set_source_rgb(r, g, b)
    colorCtx.fill()  
    colorSurf.write_to_png("games/seedwave/assets/images/" + file)
     

parts(8, "1")
parts(14, "2")
parts(20, "3")

circle(16, 0.776, 0.277, 0.990, "ships/core.png")
ellipse(4, 0.990, 0.622, 0.277, 1.0, 2.0, "ships/player.png")
ellipse(2.5, 0.990, 0.622, 0.277, 1.0, 3.0, "projectiles/pl_cannon.png")

(r, g, b) = get_color(0)
circle(4, r, g, b, "projectiles/cannon.png")

(r, g, b) = get_color(2)
ellipse(3, r, g, b, 1.0, 3.0, "projectiles/missile.png")

(r, g, b) = get_color(1)
box(16, 360, r, g, b, "projectiles/beam_3.png")
box(12, 360, r, g, b, "projectiles/beam_2.png")
box(8, 360, r, g, b, "projectiles/beam_1.png")
box(1, 360, r, g, b, "projectiles/beam_0.png")

dark = color(0.1, 0.1, 0.1)
light = color(0.13, 0.13, 0.13)
checkerboard(640, 360, 40, dark, light, "background/base.png")

mid = color(0.15, 0.15, 0.15)
sidewalls(640, 360, 20, mid,  "background/side.png")
