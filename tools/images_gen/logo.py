import cairo
import math
import sys
import os

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


def save_scaled_surface(source_surface: cairo.Surface,
                        base_width: int,
                        base_height: int,
                        target_width: int,
                        target_height: int,
                        path: str):
    scaled_surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, target_width, target_height)
    scaled_ctx = cairo.Context(scaled_surface)
    scaled_ctx.scale(target_width / base_width, target_height / base_height)
    scaled_ctx.set_source_surface(source_surface, 0, 0)
    scaled_ctx.paint()
    scaled_surface.write_to_png(path)


def create_base_icon(size: int, with_rounding: bool = True):
    R = 0.18 * size
    r = 0.1 * size if with_rounding else 0
    steps = 5
    thickness = (size / steps) * math.sqrt(2.0)
    w = (size / 2) * math.sqrt(2.0)
    h = (size / 2) * math.sqrt(2.0)


    toColor = int_to_rgba(3187733247)
    fromColor = int_to_rgba(4289593599)

    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, size, size)
    ctx = cairo.Context(surface)

    ctx.translate(size / 2, size / 2)
    ctx.rotate(math.radians(45))
    ctx.set_line_width(thickness + 2) # slight overlap to avoid gaps  

    x = -w + thickness * 0.5 # thickness + 39 * unit
    y = -h

    for i in range(steps):
        t = i / steps
        ctx.set_source_rgb(fromColor[0] + t * (toColor[0] - fromColor[0]),
                           fromColor[1] + t * (toColor[1] - fromColor[1]),
                           fromColor[2] + t * (toColor[2] - fromColor[2]))
        ctx.move_to(x, y)
        ctx.line_to(x, y + h * 2)
        ctx.stroke()
        x += thickness


    # Reset transformations
    ctx.identity_matrix()
    ctx.translate(size / 2, size / 2)
    ctx.scale(1, -1)

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

    return surface


def save_macos_icons():
    """Generate macOS app icons in all required sizes without rounded corners."""
    output_dir = "platforms/apple/shared/Assets.xcassets/AppIcon macOS.appiconset"
    os.makedirs(output_dir, exist_ok=True)

    # macOS requires these unique sizes (Contents.json handles @1x and @2x reuse)
    sizes = [
        16,    # Used for 16x16 @1x
        32,    # Used for 16x16 @2x and 32x32 @1x
        64,    # Used for 32x32 @2x
        128,   # Used for 128x128 @1x
        256,   # Used for 128x128 @2x and 256x256 @1x
        512,   # Used for 256x256 @2x and 512x512 @1x
        1024,  # Used for 512x512 @2x
    ]

    for size in sizes:
        # Create square icon without rounding (macOS adds its own)
        surface = create_base_icon(size, with_rounding=False)
        filename = f"macos_{size}.png"
        path = os.path.join(output_dir, filename)
        surface.write_to_png(path)
        print(f"Generated: {path}")


def save_ios_icons():
    """Generate iOS app icon (single 1024x1024 for modern iOS) without rounded corners."""
    output_dir = "platforms/apple/shared/Assets.xcassets/AppIcon iOS.appiconset"
    os.makedirs(output_dir, exist_ok=True)

    # iOS uses a single 1024x1024 icon without rounding (iOS adds its own)
    surface = create_base_icon(1024, with_rounding=False)
    path = os.path.join(output_dir, "ios.png")
    surface.write_to_png(path)
    print(f"Generated: {path}")


def save_generic_icons():
    """Generate generic icon sizes in resources/images with rounded corners."""
    output_dir = "resources/images"
    os.makedirs(output_dir, exist_ok=True)

    # Save main icon with rounding for generic use
    surface = create_base_icon(256, with_rounding=True)
    path = os.path.join(output_dir, "ios.png")
    surface.write_to_png(path)
    print(f"Generated: {path}")


def save_nx_icon():
    """Generate Nintendo Switch icon (1024x1024 BMP format) without rounded corners."""
    output_dir = "platforms/nx/build"
    os.makedirs(output_dir, exist_ok=True)

    # Create 1024x1024 icon without rounding for Nintendo Switch
    surface = create_base_icon(1024, with_rounding=False)
    
    # Save as PNG first
    png_path = os.path.join(output_dir, "icon_temp.png")
    surface.write_to_png(png_path)
    
    # Convert PNG to BMP using PIL/Pillow
    try:
        from PIL import Image
        img = Image.open(png_path)
        bmp_path = os.path.join(output_dir, "icon.bmp")
        img.save(bmp_path, "BMP")
        print(f"Generated: {bmp_path}")
        
        # Clean up temporary PNG
        os.remove(png_path)
    except ImportError:
        print("Error: PIL/Pillow is required to generate BMP files")
        print("Install it with: pip install Pillow")
        os.rename(png_path, os.path.join(output_dir, "icon.png"))
        print(f"Saved as PNG instead: {os.path.join(output_dir, 'icon.png')}")
    except Exception as e:
        print(f"Error converting to BMP: {e}")
        if os.path.exists(png_path):
            os.rename(png_path, os.path.join(output_dir, "icon.png"))
            print(f"Saved as PNG instead: {os.path.join(output_dir, 'icon.png')}")

# Parse command line arguments
if len(sys.argv) < 2:
    print("Usage: python logo.py <platform>")
    print("Platforms: macos, ios, nx, generic")
    sys.exit(1)

platform = sys.argv[1].lower()

# Generate icons based on platform
if platform == "macos":
    print("Generating macOS icons (square, no rounding)...")
    save_macos_icons()
elif platform == "ios":
    print("Generating iOS icons (square, no rounding)...")
    save_ios_icons()
elif platform == "nx":
    print("Generating Nintendo Switch icon (1024x1024 BMP)...")
    save_nx_icon()
elif platform == "generic":
    print("Generating all generic...")
    save_generic_icons()

else:
    print(f"Unknown platform: {platform}")
    print("Valid platforms: macos, ios, nx, generic")
    sys.exit(1)

print("Done!")
