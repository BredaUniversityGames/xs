import cairo
import colorsys

# 4x4 grid of filled squares, edge to edge, same hues as shapes_grid.py
# but with a different saturation per row
cols = 10
rows = 4
cell = 16
width = cols * cell
height = rows * cell

hues = [c / cols for c in range(cols)]
saturations = [1.0, 0.75, 0.5, 0.25]
value = 0.95

surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
ctx = cairo.Context(surface)

for row in range(rows):
    s = saturations[row % len(saturations)]
    for col in range(cols):
        h = hues[col]
        r, g, b = colorsys.hsv_to_rgb(h, s, value)

        ctx.rectangle(col * cell, row * cell, cell, cell)
        ctx.set_source_rgb(r, g, b)
        ctx.fill()

surface.write_to_png("resources/images/color_grid.png")
