---
layout: default
title: Render
nav_order: 3
---

# Render
{: .no_toc }
xs provides functionality for rendering images, texts and a number of shapes.

## Table of contents
{: .no_toc .text-delta }

1. TOC
{:toc}

## Working with bitmaps

Rendering bitmaps (sprites) is the main way of putting graphics on the screen with xs.
```csharp
import "xs" for Render

class Game {
//...
  static initialize() {
    
  } 

  static render() {
    // Loads an image from the games/<gamename>/textures folder
    var img = Render.loadImage("[game]/textures/flower.png")
    // Creates a sprite by taking a part (the bottom lower corner )
    var spr = Render.createSprite(img, 0.0, 0.0, 0.5, 0.5)
    // Renders the flower sprite 
    Render.sprite(spr, 100, 100, 2.0, Math.pi * 0.25,
      0xFFDD00FF, 0xDDFF00FF, Render.spriteCenter)
  }
//...
}
``` 

### loadImage(path)
Loads an image using a realtive path. 

### getImageWidth(imageId)
Gets the width of an loaded image

### getImageHeight(imageId)
Gets the height of an loaded image

### createSprite(imageId, x0, y0, x1, y1)
Will create a new sprite from a part of an image, with the provided the texture coordinates 

### sprite(spriteId, x, y, z, scale, rotation, mul, add, flags)
Renders a sprite on the screen, takin the original size of the sprite, given a set of parameters:
- `spriteId` - Valid sprite ID, made with `createSprite` *(not an image ID)* 
- `x` and `y` - Position for this sprite on the screen, taking the *offset* into account
- `z` sorting (depth) value
- `scale` - Scaling factor
- `rotation` - Rotation in *radians*
- `mul` - Color (represented as an number) that will be multiplied with the  values of the sprite
- `add` - Color (represented as an number) that will be added to the values of the sprite
- `flags` - Any combination of rendering bit flags
  - `spriteBottom` - Anchored at the bottom left
  - `spriteCenter` - Anchored at the center
  - `spriteFlipX` - Flip the sprite horizontally 
  - `spriteFlipY` - Flip the sprite vertically

### sprite(spriteId, x, y, z, scale, rotation, mul, add, flags)
This is equivalent to calling

`sprite(spriteId, x, y, 0.0, scale, rotation, mul, add, flags)`

When two sprites are rendered at the same z (sorting) value, they will be rendered according to call order.  

### sprite(spriteId, x, y)
This is equivalent to calling:

`sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)`

### setOffset(x, y)
Will offset all render calls with the given values, until now values are set. 

### loadFont(font, size)
Loads a given font with a certain size into a font atlas. 

### text(fontId, text, x, y, z, mul, add, flags)
Renders a text string on the screen, taking the original size of the loaded font into account, given a set of parameters. They are the same as the sprite parameters.
 - Known issue, the always text renders above the sprites. 

## Working with shapes
Shapes are rendered after all sprites have been finished rendering.

### dbgColor(r, g, b, a)
Sets the rendering color for consecutive shapes. Can also be set per vertex, for multicolored shapes. Values are in the `[0,1]` range.

### dbgColor(r, g, b)
Equivalent to calling `Render.dbgColor(r, g, b, 1)`

### dbgColor(color)
Sets the color as a single 32bit number. Example:
```csharp
Render.dbgColor(0xF0C0D0FF)
//                ^ ^ ^ ^
//                R G B A
```

### line(x0, y0, x1, y1)
Draws a line from `[x0,y0]` to `[x1,y1]`

### shapeText(text, x, y, size)
Draws a shape text on the screen with a given size

### begin(primitive)
Will start the rendering of a new primitive. This can be either `triangles` or `lines`. Provide vertices using `vertex(x, y)` and call `end()` when done to finish the the primitive. It's not possible to draw the primitives within a `being()`/`end()` block.

### end()
Finish rendering a primitive. Providing number of vertices that does not match the type (divisible with 3 for `triangles` and divisible by 2 for `lines`) will result in an error.

### vertex(x, y)
Provide a vertex. This call only be called between `being()` and `end()`. 

