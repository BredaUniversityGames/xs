#!/usr/bin/env python3
"""Generate simple placeholder assets for the survivors-like game"""

from PIL import Image, ImageDraw
import math

def create_circle(size, filename):
    """Create a white circle on transparent background"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.ellipse([0, 0, size-1, size-1], fill=(255, 255, 255, 255))
    img.save(filename)
    print(f"Created {filename}")

def create_triangle(size, filename):
    """Create a white triangle pointing right on transparent background"""
    img = Image.new('RGBA', (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    # Triangle pointing right
    points = [
        (size * 0.1, size * 0.1),           # top left
        (size * 0.9, size * 0.5),           # right point
        (size * 0.1, size * 0.9)            # bottom left
    ]
    draw.polygon(points, fill=(255, 255, 255, 255))
    img.save(filename)
    print(f"Created {filename}")

def create_bullet(filename):
    """Create a small white bullet (elongated circle)"""
    width, height = 16, 8
    img = Image.new('RGBA', (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    draw.ellipse([0, 0, width-1, height-1], fill=(255, 255, 255, 255))
    img.save(filename)
    print(f"Created {filename}")

# Generate enemy discs of different sizes
radii = [50, 60, 70, 80, 90, 100, 110, 120]
for radius in radii:
    size = radius * 2
    create_circle(size, f"images/disc_{radius}.png")

# Generate player triangle
create_triangle(40, "images/player.png")

# Generate bullet
create_bullet("images/bullet.png")

# Generate a simple enemy triangle (smaller, pointing left)
create_triangle(30, "images/enemy.png")

print("All assets generated successfully!")
