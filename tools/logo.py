import cairo
import math

def int_to_rgb(i):
    return ((i >> 16) & 0xff) / 255, ((i >> 8) & 0xff) / 255, (i & 0xff) / 255

def int_to_rgba(i):
    return ((i >> 24) & 0xff) / 255, ((i >> 16) & 0xff) / 255, ((i >> 8) & 0xff) / 255, (i & 0xff) / 255
    
def round_rectangle(ctx : cairo.Context,
               x : float,
               y : float,
               w : float,
               h : float,
               r : float):
    ctx.move_to(x + r, y)
    ctx.arc(x + w - r, y + r, r, 3 * math.pi / 2, 0)
    ctx.arc(x + w - r, y + h - r, r, 0, math.pi / 2)
    ctx.arc(x + r, y + h - r, r, math.pi / 2, math.pi)
    ctx.arc(x + r, y + r, r, math.pi, 3 * math.pi / 2)
    ctx.close_path()    

# Set the radius of the circle and the rounding radius
scale = 1.0 / 2
width = int(256 * scale)
height = int(256 * scale)
R = int(44 * scale)
r = int(18 * scale)
thickness = 30 * scale
steps = 13
w = width / 2
h = height / 2

surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
ctx = cairo.Context(surface)

# Move to the center of the surface
ctx.translate(width / 2, height / 2)
ctx.scale(1, -1)

# draw a rectangle in the middle
ctx.set_source_rgb(1, 1, 1)
round_rectangle(ctx, -w, -h, width, height, r)
ctx.clip()

# Draw 45 degree lines in the logo
ctx.set_line_width(thickness)

x = thickness + 40 * scale
y = -w - thickness - 0.5
fromColor = int_to_rgba(3187733247)
toColor = int_to_rgba(4289593599)
for i in range(steps):
    t = i / steps
    ctx.set_source_rgb(fromColor[0] + t * (toColor[0] - fromColor[0]),
                       fromColor[1] + t * (toColor[1] - fromColor[1]),
                       fromColor[2] + t * (toColor[2] - fromColor[2]))
    ctx.move_to(x, y)
    ctx.line_to(x + width * 2, y + width * 2)
    ctx.stroke()
    x -= thickness * math.sqrt(2) - 1

# Set to white
ctx.set_source_rgb(1, 1, 1)

# Draw the half circles of the logo
# bottom of the x
x = -1.5 * R
y = -R
ctx.arc(x, y, R, 0, math.pi)
ctx.close_path()
ctx.fill()
# top of the x
x = -1.5*R
y = R
ctx.arc(x, y, R, math.pi, 2 * math.pi)
ctx.close_path()
ctx.fill()
# bottom of the s
x =  0.5 * R
y = 0
ctx.arc(x, y, R, math.pi, 2 * math.pi)
ctx.close_path()
ctx.fill()
# top of the s
x = 1.5 * R
y = 0
ctx.arc(x, y, R, 0, math.pi)
ctx.close_path()
ctx.fill()

# Save the image as a PNG file
surface.write_to_png("games/shared/images/icon_small.png")
