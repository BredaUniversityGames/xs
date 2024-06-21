import cairo
import random
import colorsys
import math
from xsmath import vec2 

def circle(ctx : cairo.Context,
           x : float,
           y : float,
           r : float,
           color : tuple) :
    ctx.set_source_rgb(color[0], color[1], color[2])
    ctx.arc(x, y, r, 0, 2 * math.pi)
    ctx.fill()

def circle_stroke(ctx : cairo.Context,
            x : float,
            y : float,
            r : float,
            thickness : float,
            color : tuple) :
    ctx.set_source_rgb(color[0], color[1], color[2])
    ctx.arc(x, y, r, 0, 2 * math.pi)
    ctx.set_line_width(thickness)
    ctx.stroke()

def capsule(ctx : cairo.Content,
            x0 : float,
            y0 : float,
            x1 : float,
            y1 : float,
            radius : float,
            color : tuple) :
    ctx.set_source_rgb(color[0], color[1], color[2])
    ctx.set_line_width(2 * radius)
    ctx.move_to(x0, y0)
    ctx.line_to(x1, y1)
    ctx.stroke()
    ctx.arc(x0, y0, radius, 0, 2 * math.pi)
    ctx.arc(x1, y1, radius, 0, 2 * math.pi)
    ctx.fill()

# Set the radius of the circle and the rounding radius
scale = 0.25
R = 64 * scale
r = 20 * scale 
Rr = R + r
RR = Rr + r
expand = 40 * scale

# Set the flag to render lines
render_lines = False

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

color_bg = purple
color_bg2 = darker_purple
color_bg3 = dark_purple
color_letters = white

# Create a surface and a context
width = int(4 * RR + expand)
height = width #int(2 * RR + expand) 
surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
ctx = cairo.Context(surface)

# Move to the center of the surface
ctx.translate(width / 2, height / 2)
ctx.scale(1, -1)
# Get ready
srqt2 = math.sqrt(2)
a45 = math.pi / 4
a135 = 3 * math.pi / 4
a225 = 5 * math.pi / 4
a315 = 7 * math.pi / 4
a90 = math.pi / 2
a180 = math.pi
a270 = 3 * math.pi / 2


# Draw a capusle as background for the two letters
#capsule(ctx, -Rr, 0, Rr, 0, RR, orange)
#capsule(ctx, -Rr, 0, Rr, 0, Rr, dark_orange)
#capsule(ctx, -Rr, 0, Rr, 0, R, orange)
#capsule(ctx, -Rr, 0, Rr, 0, R - r, dark_orange)


CR = RR + r * 2
c = 0.2
# an array of colors that interpolates between the purple and dark blue in 4 steps
colors = [
    (0.5, 0, 0.5),
    (0.25, 0, 0.35),
    (0.15, 0, 0.35),
    (0.0, 0, 0.35)
]
for i in range(0, 4):
    # color = (c, 0, c)
    color = colors[3-i]
    #circle(ctx, -Rr, 0, CR, color)
    #circle(ctx, Rr, 0, CR, color)    
    #d = RR
    #mr = vec2(-Rr, 0) - vec2(0, d)
    #mr = mr.magnitude() - d - r
    #lw = 2 * r
    #ctx.set_line_width(lw)
    #ctx.set_source_rgb(color_bg2[0], color_bg2[1], color_bg2[2])
    #ctx.arc(0, d, mr + 0.5 * lw, a225, a315)
    #ctx.stroke()
    #ctx.arc(0, -d, mr + 0.5 * lw, a45, a135)
    #ctx.stroke()
    capsule(ctx, -Rr, 0, Rr, 0, CR, color)
    CR -= r
    c += 0.1

'''
circle(ctx, -Rr, 0, RR, color_bg)
circle(ctx, Rr, 0, RR, color_bg)
mr = vec2(-Rr, 0) - vec2(0, RR)
mr = mr.magnitude() - RR
lw = 2 * r
ctx.set_line_width(lw)
ctx.set_source_rgb(color_bg[0], color_bg[1], color_bg[2])
ctx.arc(0, RR, mr + 0.5 * lw, a225, a315)
ctx.stroke()
ctx.arc(0, -RR, mr + 0.5 * lw, a45, a135)
ctx.stroke()
'''


# Draw the s in the second circle

# Move the context to the right
ctx.save()
ctx.translate(Rr, 0)
ctx.rotate(0.3)
ctx.scale(0.95, 0.95)

# Update the key points
# R = R - r
A = vec2(R / srqt2, R / srqt2)
D = vec2(-R / srqt2, -R / srqt2)
B = vec2(0, R)
C = vec2(0, -R)
B0 = vec2(0, R / 2)
C0 = vec2(0, -R / 2)

# dark_orange = (0.75, 0.45, 0)

ctx.set_source_rgb(color_letters[0], color_letters[1], color_letters[2])
ctx.set_line_width(2*r)
ctx.arc(0, 0, R, a45, a90)
ctx.arc(B0.x, B0.y, R / 2, a90, a270)
ctx.stroke()
ctx.arc(0, 0, R, a225, -a90)
ctx.arc(C0.x, C0.y, R / 2,  -a90, a90)
ctx.stroke()
circle(ctx, A.x, A.y, r, color_letters)
circle(ctx, D.x, D.y, r, color_letters)

if render_lines :
    circle_stroke(ctx, 0, 0, R, 1.0, white)
    circle_stroke(ctx, 0, 0, r, 1.0, white)
    circle_stroke(ctx, 0, 0, Rr, 1.0, white)
    circle_stroke(ctx, A.x, A.y, r, 1.0, white)
    circle_stroke(ctx, B.x, B.y, r, 1.0, white)
    circle_stroke(ctx, C.x, C.y, r, 1.0, white)
    circle_stroke(ctx, D.x, D.y, r, 1.0, white)
    circle_stroke(ctx, B0.x, B0.y, r, 1.0, white)
    circle_stroke(ctx, C0.x, C0.y, r, 1.0, white)

# Restore the context
ctx.restore()

# Draw an X in the first circle as two capsules

# first move the context to the center of the first circle
# and flip the y-axis
ctx.save()
ctx.translate(-Rr, 0)
A = vec2(R / srqt2, R / srqt2)
B = vec2(-R / srqt2, R / srqt2)
C = vec2(R / srqt2, -R / srqt2)
D = vec2(-R / srqt2, -R / srqt2)
AB = vec2(0, 4 * r / srqt2)
BC = vec2(-4 * r / srqt2, 0)
CD = vec2(0, -4 * r / srqt2)
DA = vec2(4 * r / srqt2, 0)


capsule(ctx, A.x, A.y, D.x, D.y, r, color_letters)    # top left to bottom right
capsule(ctx, B.x, B.y, C.x, C.y, r, color_letters)    # top right to bottom left
ctx.set_source_rgb(color_letters[0], color_letters[1], color_letters[2])
ctx.set_line_width(2*r)
ctx.arc(AB.x, AB.y, 2 * r, a225, a315)
ctx.stroke()
ctx.arc(BC.x, BC.y, 2 * r, a315, a45)
ctx.stroke()
ctx.arc(CD.x, CD.y, 2 * r, a45, a135)
ctx.stroke()
ctx.arc(DA.x, DA.y, 2 * r, a135, a225)
ctx.stroke()

if render_lines :
    circle_stroke(ctx, 0, 0, R, 1.0, white)
    circle_stroke(ctx, 0, 0, r, 1.0, white)
    circle_stroke(ctx, 0, 0, Rr, 1.0, white)
    circle_stroke(ctx, A.x, A.y, r, 1.0, white)
    circle_stroke(ctx, B.x, B.y, r, 1.0, white)
    circle_stroke(ctx, C.x, C.y, r, 1.0, white)
    circle_stroke(ctx, D.x, D.y, r, 1.0, white)
    #circle_stroke(ctx, AB.x, AB.y, r, 1.0, white)
    #circle_stroke(ctx, BC.x, BC.y, r, 1.0, white)
    #circle_stroke(ctx, CD.x, CD.y, r, 1.0, white)
    #circle_stroke(ctx, DA.x, DA.y, r, 1.0, white)
    ctx.arc(AB.x, AB.y, r, a225, a315)
    ctx.stroke()
    ctx.arc(BC.x, BC.y, r, a315, a45)
    ctx.stroke()
    ctx.arc(CD.x, CD.y, r, a45, a135)
    ctx.stroke()
    ctx.arc(DA.x, DA.y, r, a135, a225)
    ctx.stroke()


# Save the image as a PNG file
surface.write_to_png("logo.png")




