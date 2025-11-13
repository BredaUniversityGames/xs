import cairo
import math
import xml.etree.ElementTree as ET

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

# Create SVG surface
surface = cairo.SVGSurface("assets/images/icon_small_animated.svg", width, height)
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

# Finish and close the SVG surface
surface.finish()

# Now post-process the SVG to add animations
svg_file = "assets/images/icon_small_animated.svg"
tree = ET.parse(svg_file)
root = tree.getroot()

# Define namespace
ns = {'svg': 'http://www.w3.org/2000/svg'}
ET.register_namespace('', 'http://www.w3.org/2000/svg')

# Get all path elements (the half circles)
paths = root.findall('.//svg:path', ns)

if len(paths) >= 4:
    # Calculate animation values
    # X circles: move vertically to merge (offset = R = 22)
    x_vertical_offset = R
    
    # S circles: move horizontally to merge (half of R = 11)
    s_horizontal_offset = R / 2
    
    # Remove all clipPath elements to prevent bounding box constraints
    defs = root.find('.//svg:defs', ns)
    if defs is not None:
        for clippath in root.findall('.//svg:clipPath', ns):
            defs.remove(clippath)
    
    # Remove all clip-path attributes from groups
    for group in root.findall('.//{http://www.w3.org/2000/svg}g', ns):
        if 'clip-path' in group.attrib:
            del group.attrib['clip-path']
    
    # Add animations to each half circle with longer pause times
    # keyTimes: 0 (start at initial), 0.35 (pause at initial), 0.65 (pause at merged), 1 (back to initial)
    # dur: 3s total (gives lots of pause time)
    
    # Bottom of X (path index should be after the gradient lines)
    if len(paths) > 0:
        x_bottom = paths[-4]  # Last 4 paths are the half circles
        anim = ET.Element('{http://www.w3.org/2000/svg}animateTransform')
        anim.set('attributeName', 'transform')
        anim.set('type', 'translate')
        anim.set('values', f"0,0; 0,0; 0,-{x_vertical_offset}; 0,-{x_vertical_offset}; 0,0; 0,0")
        anim.set('dur', '3s')
        anim.set('keyTimes', '0; 0.25; 0.35; 0.65; 0.75; 1')
        anim.set('repeatCount', 'indefinite')
        anim.set('calcMode', 'spline')
        anim.set('keySplines', '0.42 0 0.58 1; 0.42 0 0.58 1; 0 0 0 0; 0.42 0 0.58 1; 0.42 0 0.58 1')
        anim.set('additive', 'sum')
        x_bottom.append(anim)
    
    # Top of X
    if len(paths) > 1:
        x_top = paths[-3]
        anim = ET.Element('{http://www.w3.org/2000/svg}animateTransform')
        anim.set('attributeName', 'transform')
        anim.set('type', 'translate')
        anim.set('values', f"0,0; 0,0; 0,{x_vertical_offset}; 0,{x_vertical_offset}; 0,0; 0,0")
        anim.set('dur', '3s')
        anim.set('keyTimes', '0; 0.25; 0.35; 0.65; 0.75; 1')
        anim.set('repeatCount', 'indefinite')
        anim.set('calcMode', 'spline')
        anim.set('keySplines', '0.42 0 0.58 1; 0.42 0 0.58 1; 0 0 0 0; 0.42 0 0.58 1; 0.42 0 0.58 1')
        anim.set('additive', 'sum')
        x_top.append(anim)
    
    # Bottom of S
    if len(paths) > 2:
        s_bottom = paths[-2]
        anim = ET.Element('{http://www.w3.org/2000/svg}animateTransform')
        anim.set('attributeName', 'transform')
        anim.set('type', 'translate')
        anim.set('values', f"0,0; 0,0; {s_horizontal_offset},0; {s_horizontal_offset},0; 0,0; 0,0")
        anim.set('dur', '3s')
        anim.set('keyTimes', '0; 0.25; 0.35; 0.65; 0.75; 1')
        anim.set('repeatCount', 'indefinite')
        anim.set('calcMode', 'spline')
        anim.set('keySplines', '0.42 0 0.58 1; 0.42 0 0.58 1; 0 0 0 0; 0.42 0 0.58 1; 0.42 0 0.58 1')
        anim.set('additive', 'sum')
        s_bottom.append(anim)
    
    # Top of S
    if len(paths) > 3:
        s_top = paths[-1]
        anim = ET.Element('{http://www.w3.org/2000/svg}animateTransform')
        anim.set('attributeName', 'transform')
        anim.set('type', 'translate')
        anim.set('values', f"0,0; 0,0; -{s_horizontal_offset},0; -{s_horizontal_offset},0; 0,0; 0,0")
        anim.set('dur', '3s')
        anim.set('keyTimes', '0; 0.25; 0.35; 0.65; 0.75; 1')
        anim.set('repeatCount', 'indefinite')
        anim.set('calcMode', 'spline')
        anim.set('keySplines', '0.42 0 0.58 1; 0.42 0 0.58 1; 0 0 0 0; 0.42 0 0.58 1; 0.42 0 0.58 1')
        anim.set('additive', 'sum')
        s_top.append(anim)

# Save the modified SVG
tree.write(svg_file, encoding='utf-8', xml_declaration=True)
print(f"Animated SVG created: {svg_file}")
