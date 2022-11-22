#!/usr/bin/env python

import cairo
import random
import colorsys
import math

seed = 1002797
random.seed(seed)

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


width, height = 640, 360
base = random.random() * 0.3 + 0.4

def main():
    finalSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    finalCtx = cairo.Context(finalSurf)
    ctx = cairo.Matrix(1, 0, 0, -1, 0, height)
    finalCtx.set_matrix(ctx)
    finalCtx.set_source_rgba(1,1,1)
    finalCtx.move_to(0, 0)
    for x in range(0, int(width / 2)):
        h = int(abs(math.sin(x / 30)) * 100)
        finalCtx.rectangle(2 * x, 0, 2, h)
    ##roundrect(finalCtx, 10, 10, width-20, height-20, 40)
    #finalCtx.fill()
    #roundrect(finalCtx, 5, 5, width - 10, height - 10, 45)
    #finalCtx.set_line_width(10)
    #(h, s, v) = (base + 0.28, 0.6, 0.9)
    #(r, g, b) = colorsys.hsv_to_rgb(h, s, v)
    #finalCtx.set_source_rgb(r,g,b)
    #finalCtx.stroke()
    #finalCtx.rectangle(0, 0, width, height)
    finalCtx.fill()

    finalSurf.write_to_png("games/jump-core/images/city.png")
    print("Done")

main()
