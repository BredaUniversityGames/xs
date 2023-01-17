#!/usr/bin/env python

import cairo
import random
import colorsys
import math

random.seed()

width, height = 720, 720

colorSurf = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
colorCtx = cairo.Context(colorSurf)
total = 100
sqt = int(math.sqrt(total))
dx = width / sqt
dy = height / sqt
gx = 0
gy = 0
for i in range(0, sqt):
    for j in range(0, sqt):
        (h, s, v) = (random.random(), 0.2, 0.8 + random.random() * 0.2)
        (r, g, b) = colorsys.hsv_to_rgb(h, s, v)        
        colorCtx.set_source_rgb(r, g, b)    
        #colorCtx.set_source_rgb(1, 1, 1)
        x = gx + random.random() * dx
        y = gy + random.random() * dy
        s = random.randint(1,3)
        colorCtx.rectangle(int(x), int(y), s, s)
        colorCtx.fill()
        gx += dx
    gx = 0
    gy += dy

colorSurf.write_to_png("games/samples/shootly/assets/images/Background/stars.png")

print("Done")
