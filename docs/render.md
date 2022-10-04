---
layout: default
title: Render
nav_order: 2
---

# Render
Provides functionality for rendering images, texts and a number of shapes. 

## Working with bitmaps

Rendering bitmaps (sprites) is the main 
Example

### **loadImage**(path)
Loads an image using a realtive path. 

### **getImageWidth**(imageId)
Gets the width of an loaded image

### **getImageHeight**(imageId)
Gets the height of an loaded image

### **createSprite**(imageId, x0, y0, x1, y1)
Will create a new sprite from a part of an image, provided the 

### **renderSprite**(spriteId, x, y, scale, rotation, mul, add, flags)
Renders a sprite on the screen, takin the original size of the sprite, given a set of paramters:
- `spriteId` - Valid sprite ID, made with `createSprite` *(not an image ID)* 
- `x` and `y` - Position for this sprite on the screen, taking the *offset* into account
- `scale` - Scaling factor
- `rotation` - Rotation in *radians*
- `mul` - Color (represented as an number) that will be multiplied with the  values of the sprite
- `add` - Color (represented as an number) that will be added to the values of the sprite
- `flags` - Any combination of rendering bit flags
  - `spriteBottom` - Anchored at the bottom left
  - `spriteCenter` - Anchored at the center
  - `spriteFlipX` - Flip the sprite horizontaly 
  - `spriteFlipY` - Flip the sprite verticaly
### **renderSprite**(spriteId, x, y)
Equivalent to calling 

`renderSprite(spriteId, x, y, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)`

### **setOffset**(x, y)

### **loadFont**(font, size)

### **renderText**(fontId, text, x, y, mul, add, flags)


## Working with shapes
Example
Renders after all sprite have been rendered.

### **setColor**(r, g, b, a)
### **setColor**(r, g, b)
Equivalent to calling

`Render.setColor(r, g, b, 1)`

### **setColor**(color)
Sets the color as a single 32bit number.

### **line**(x0, y0, x1, y1)

### **text**(text, x, y, size)

### **begin**(primitive)
Will start the rendering of a new primitive. This can be either `triangles` or `lines`. Provide vertices using `vertex(x, y)` and call `end()` when done to finish the the primitive.

### **end**()
Finish rendering a primitive. Providing number of vertices that does not match the type (divisible with 3 for `triangles` and divisible by 2 for `lines`) will result in an error.

### **vertex**(x, y)
Provide a vertex. This call only be called between `being()` and `end()`

