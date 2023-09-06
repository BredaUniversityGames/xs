#!/usr/bin/env python

import cairo
import random
import colorsys
import math
from xsmath import vec2 

def next_power_of_2(x):  
    return 1 if x == 0 else 2**(x - 1).bit_length()

def reticle(radius):
    number = 32
    next_p2 = next_power_of_2(radius * 2 + 1)
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * number, next_p2)    
    colorCtx = cairo.Context(colorSurf)
    colorCtx.set_antialias(cairo.Antialias.NONE)
    (r, g, b) = (1,1,1)        
    colorCtx.set_source_rgb(r, g, b)
    third = 1.0 / 3.0
    colorCtx.set_line_width(2.5)
    for i in range(0, number):
        x = i * next_p2 + next_p2 * 0.5
        y = next_p2 * 0.5
        angle = math.pi * 2.0 * float(i) / number

        colorCtx.arc(x, y, radius, angle, angle + math.pi * third)
        colorCtx.stroke()
        
        angle += math.pi * third * 2.0
        colorCtx.arc(x, y, radius, angle, angle + math.pi * third)
        colorCtx.stroke()
        
        angle += math.pi * third * 2.0
        colorCtx.arc(x, y, radius, angle, angle + math.pi * third)
        colorCtx.stroke()
    colorSurf.write_to_png("games/lock-on/images/ui/reticle.png")

def lock(radius):
    number = 32
    next_p2 = next_power_of_2(radius * 2 + 1)
    colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, next_p2 * (number + 1), next_p2)    
    colorCtx = cairo.Context(colorSurf)
    colorCtx.set_antialias(cairo.Antialias.NONE)
    (r, g, b) = (1,1,1)        
    colorCtx.set_source_rgb(r, g, b)    
    colorCtx.set_line_width(2.5)
    for i in range(0, number + 1):
        x = i * next_p2 + next_p2 * 0.5
        y = next_p2 * 0.5
        angle = math.pi * 2.0 * float(i) / number

        colorCtx.arc(x, y, radius, 0.0, angle)        
        colorCtx.stroke()
                
        '''
        progress = float(i) / number * 100.0
        progress_str = "{:.0f}%".format(progress)
        colorCtx.set_font_size(radius * 0.4)
        colorCtx.select_font_face("Mono", cairo.FONT_SLANT_NORMAL, cairo.FONT_WEIGHT_BOLD)
        colorCtx.set_source_rgb(1, 1, 1)
        (xb, yb, w, h, dx, dy) = colorCtx.text_extents(progress_str)
        colorCtx.move_to(x + next_p2 * 0.25, next_p2 * 0.15)
        colorCtx.show_text(progress_str)
        colorCtx.fill()
        '''
    colorSurf.write_to_png("games/lock-on/images/ui/lock.png")

reticle(12)
lock(24)