import cairo
import random
import colorsys
import math

def int_to_rgb(i):
    return ((i >> 16) & 0xff) / 255, ((i >> 8) & 0xff) / 255, (i & 0xff) / 255

def circle(ctx : cairo.Context,
           x : float,
           y : float,
           r : float,
           color : tuple) :
    ctx.set_source_rgb(color[0], color[1], color[2])
    ctx.arc(x, y, r, 0, 2 * math.pi)
    ctx.fill()

def round_rectangle(ctx : cairo.Context,
               x : float,
               y : float,
               w : float,
               h : float,
               r : float,
               color : tuple) :
    ctx.set_source_rgb(color[0], color[1], color[2])
    ctx.move_to(x + r, y)
    ctx.arc(x + w - r, y + r, r, 3 * math.pi / 2, 0)
    ctx.arc(x + w - r, y + h - r, r, 0, math.pi / 2)
    ctx.arc(x + r, y + h - r, r, math.pi / 2, math.pi)
    ctx.arc(x + r, y + r, r, math.pi, 3 * math.pi / 2)
    ctx.close_path()    

# Set the radius of the circle and the rounding radius
scale = 1
width = int(256 * scale)
height = int(256 * scale)
R = int(64 * scale)
r = int(18 * scale)
thickness = 30 * scale
steps = 12
w = width / 2
h = height / 2

# Define the colors
orange = (1, 0.5, 0)
blue = (0, 0, 1)
white = (1, 1, 1)
dark_orange = (0.5, 0.25, 0)
dark_orange = orange
purple = (0.5, 0, 0.5)
yellow = (1, 1, 0)
darker_purple = (0.35, 0, 0.35)
dark_purple = (0.25, 0, 0.25)

surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
ctx = cairo.Context(surface)

# Move to the center of the surface
ctx.translate(width / 2, height / 2)
ctx.scale(1, -1)

# draw a rectangle in the middle
round_rectangle(ctx, -w, -h, width, height, r, white)
ctx.clip()

# Draw 45 degree lines in the logo
ctx.set_line_width(thickness)

x = thickness + 40
y = -w - thickness - 0.5
fromColor = blue
toColor = orange
for i in range(steps):
    t = i / steps
    ctx.set_source_rgb(fromColor[0] + t * (toColor[0] - fromColor[0]),
                       fromColor[1] + t * (toColor[1] - fromColor[1]),
                       fromColor[2] + t * (toColor[2] - fromColor[2]))
    ctx.move_to(x, y)
    ctx.line_to(x + width * 2, y + width * 2)
    ctx.stroke()
    x -= thickness * math.sqrt(2) - 1



# Save the image as a PNG file
surface.write_to_png("logo.png")
