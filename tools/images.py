import cairo
import math

# create a checkerboard image with 1280x720 pixels
width, height = 1280, 720
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
ctx = cairo.Context(surface)

dark = 0.45
light = 0.55

# draw a checkerboard pattern
for i in range(0, width, 80):
    for j in range(0, height, 80):
        if (i // 80 + j // 80) % 2 == 0:
            # set the color to dark gray
            ctx.set_source_rgb(dark, dark, dark)
        else:
            # set the color to light gray
            ctx.set_source_rgb(light, light, light)         
        ctx.rectangle(i, j, 80, 80)
        ctx.fill()

# save the image as a PNG file
surface.write_to_png("games/shared/images/checkerboard_720p.png")

