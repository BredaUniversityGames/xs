#!/usr/bin/env python

import cairo
import random
import colorsys
import math

random.seed()

width, height = 1280, 720

colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
colorCtx = cairo.Context(colorSurf)
total = 300
sqt = int(math.sqrt(total))
dx = width / sqt
dy = height / sqt
gx = 0
gy = 0
for i in range(0, sqt):
    for j in range(0, sqt):
        (h, s, v) = (random.random(), 0.1, 0.8 + random.random() * 0.2)
        (r, g, b) = colorsys.hsv_to_rgb(h, s, v)
        x = gx + random.random() * dx
        y = gy + random.random() * dy
        rnd = random.random()
        if rnd < 0.6:
            (r, g, b) = colorsys.hsv_to_rgb(h, s, v)
            colorCtx.set_source_rgb(r, g, b)
            colorCtx.rectangle(int(x), int(y), 1, 1)
            colorCtx.fill()
        elif rnd < 0.95:
            (r, g, b) = colorsys.hsv_to_rgb(h, s, v)
            colorCtx.set_source_rgb(r, g, b)
            colorCtx.rectangle(int(x - 1), int(y), 3, 1)
            colorCtx.rectangle(int(x), int(y - 1), 1, 3)
            colorCtx.fill()
            (r, g, b) = colorsys.hsv_to_rgb(h, s, 1)
            colorCtx.set_source_rgb(r, g, b)            
            colorCtx.rectangle(int(x), int(y), 1, 1)
            colorCtx.fill()
        else:
            colorCtx.rectangle(int(x - 2), int(y), 5, 1)
            colorCtx.rectangle(int(x), int(y - 2), 1, 5)
            (r, g, b) = colorsys.hsv_to_rgb(h, s, v)
            colorCtx.set_source_rgb(r, g, b)
            colorCtx.fill()
            colorCtx.rectangle(int(x - 1), int(y - 1), 3, 3)
            (r, g, b) = colorsys.hsv_to_rgb(h, s, 1)
            colorCtx.set_source_rgb(r, g, b)
            colorCtx.fill()        
        gx += dx
    gx = 0
    gy += dy

colorSurf.write_to_png("games/samples/galax/assets/images/Background/stars.png")

print("Done")
