#!/usr/bin/env python

import cairo
import random
import colorsys
import math

letters = ["c", "l", "m", "d", "n", "e", "h", "v"]

def next_power_of_2(x):  
    return 1 if x == 0 else 2**(x - 1).bit_length()

def get_color(num):
      #random.seed(num)
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
        colorCtx.set_font_size(radius * 1.07)
        colorCtx.select_font_face("Mono", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
        colorCtx.set_source_rgb(1, 1, 1)
        (xb, yb, w, h, dx, dy) = colorCtx.text_extents(l)
        #colorCtx.move_to(x - size * 0.3, y + size * 0.3)
        colorCtx.move_to(x - w * 0.4, y + h * 0.5)
        colorCtx.show_text(l)
        colorCtx.fill()

    colorSurf.write_to_png("games/seedwave/assets/images/ships/parts_" + name + ".png")


def ellipse(radius : float, r : float, g : float, b : float, sx : float, sy : float, file : str):
    next_p2 = next_power_of_2(radius * int(max(sx, sy)) * 2 + 1)
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
     

parts(8, "s")
parts(14, "m")
parts(20, "l")
circle(16, 1, 0.5, 0.5, "ships/core.png")
ellipse(4, 0.5, 0.8, 0.8, 1.0, 2.0, "ships/player.png")
circle(3, 1, 0.5, 0.8, "projectiles/cannon.png")
ellipse(3, 1, 0.5, 0.5, 1.0, 3.0, "projectiles/missile.png")
