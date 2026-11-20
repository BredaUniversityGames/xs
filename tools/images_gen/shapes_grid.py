import cairo
import colorsys
import math

# grid of simple shapes (square, circle, triangle, diamond) in different colors,
# useful as a generic placeholder / test texture
cols = 10
rows = 3
cell = 56
width = cols * cell
height = rows * cell

shapes = ["square", "circle", "triangle"]
colors = [colorsys.hsv_to_rgb(h / cols, 0.75, 0.95) for h in range(cols)]

surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
ctx = cairo.Context(surface)


def draw_square(ctx, cx, cy, s):
    ctx.rectangle(cx - s, cy - s, s * 2, s * 2)


def draw_circle(ctx, cx, cy, s):
    ctx.arc(cx, cy, s, 0.0, math.pi * 2.0)


def draw_triangle(ctx, cx, cy, s):
    ctx.move_to(cx, cy - s)
    ctx.line_to(cx + s * math.sin(math.pi * 2 / 3), cy - s * math.cos(math.pi * 2 / 3))
    ctx.line_to(cx - s * math.sin(math.pi * 2 / 3), cy - s * math.cos(math.pi * 2 / 3))
    ctx.close_path()


draw_fns = {
    "square": draw_square,
    "circle": draw_circle,
    "triangle": draw_triangle,
}

radius = cell * 0.42

for row in range(rows):
    shape = shapes[row % len(shapes)]
    draw_fn = draw_fns[shape]
    for col in range(cols):
        cx = col * cell + cell * 0.5
        cy = row * cell + cell * 0.5
        r, g, b = colors[col]

        draw_fn(ctx, cx, cy, radius)
        ctx.set_source_rgb(r, g, b)
        ctx.fill()

surface.write_to_png("resources/images/shapes_grid.png")
