---
layout: default
title: Core API (Render, Input, etc.)
parent: API Reference
nav_order: 1
---

<!-- file: assets/modules/xs.wren -->
<!-- documentation automatically generated using domepunk/tools/doc -->
---
## [Class ShapeHandle](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L2)

Handle for shapes and sprites in the rendering system

---
## [Class Render](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L6)

Core rendering API for sprites, shapes, text, and debug drawing
Provides functionality for rendering images, texts and shapes

## API

### [static loadImage(path)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L10)

Loads an image from a file and returns an image ID
Supports PNG and JPG formats. Use relative paths like "[game]/textures/flower.png"

### [static loadShape(path)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L14)

Loads a shape from a file and returns a shape ID
Supports SVG format

### [static loadFont(font, size)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L18)

Loads a font into a font atlas and returns a font ID
Font will be rasterized at the specified size

### [static getImageWidth(imageId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L21)

Gets the width in pixels of a loaded image

### [static getImageHeight(imageId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L24)

Gets the height in pixels of a loaded image

### [static createSprite(imageId, x0, y0, x1, y1)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L28)

Creates a sprite from a section of an image using texture coordinates
Coordinates are normalized (0.0 to 1.0): x0, y0 (top-left), x1, y1 (bottom-right)

### [static createShape(imageId, positions, textureCoords, indices)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L31)

Creates a custom mesh shape from vertices, texture coordinates, and indices

### [static destroyShape(shapeId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L34)

Destroys a shape and frees its resources

### [static setOffset(x, y)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L38)

Sets the offset for subsequent sprite draw calls
All sprites will be offset by (x, y) until new values are set

### [static sprite(spriteId, x, y, z, scale, rotation, mul, add, flags)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L49)

Draws a sprite with full control over appearance
- spriteId: Valid sprite ID created with createSprite (not an image ID)
- x, y: Position on screen (affected by setOffset)
- z: Sorting depth value
- scale: Scaling factor
- rotation: Rotation angle in radians
- mul: Multiply color (0xRRGGBBAA format)
- add: Additive color (0xRRGGBBAA format)
- flags: Combination of sprite flags (spriteBottom, spriteCenter, etc.)

### [static shape(shapeId, x, y, z, scale, rotation, mul, add)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L52)

Draws a shape at a position with transformation

### [static text(fontId, txt, x, y, z, mul, add, flags)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L56)

Draws text at a position with styling
Note: Text always renders above sprites currently

### [static spriteNone       { 0 << 0 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L59)

Don't apply any flags

### [static spriteBottom     { 1 << 1 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L62)

Draw the sprite at the bottom

### [static spriteTop        { 1 << 2 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L65)

Draw the sprite at the top

### [static spriteCenterX    { 1 << 3 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L68)

Center the sprite on the x-axis

### [static spriteCenterY    { 1 << 4 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L71)

Center the sprite on the y-axis

### [static spriteFlipX      { 1 << 5 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L74)

Flip the sprite on the x-axis

### [static spriteFlipY      { 1 << 6 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L77)

Flip the sprite on the y-axis

### [static spriteFixed    { 1 << 7 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L80)

Overlay the sprite as overlay (no offset applied)

### [static spriteCenter     { spriteCenterX | spriteCenterY }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L83)

Center the sprite on the x and y-axis

### [static spriteShape      { 1 << 8 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L86)

This is not a sprite but a shape, so handle it differently

### [static lines { 0 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L89)

Primitive type for line rendering

### [static triangles { 1 }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L92)

Primitive type for triangle rendering

### [static dbgBegin(primitive)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L97)

Begins a debug primitive batch
Call dbgVertex() to add vertices, then dbgEnd() to finish
Primitive can be lines or triangles

### [static dbgEnd()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L101)

Ends a debug primitive batch and renders it
Number of vertices must match primitive type (divisible by 2 for lines, 3 for triangles)

### [static dbgVertex(x, y)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L105)

Adds a vertex to the current debug primitive
Must be called between dbgBegin() and dbgEnd()

### [static dbgColor(color)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L109)

Sets the color for the next debug vertices
Color format: 0xRRGGBBAA (e.g., 0xF0C0D0FF)

### [static dbgLine(x0, y0, x1, y1)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L112)

Draws a debug line from (x0, y0) to (x1, y1)

### [static dbgText(text, x, y, size)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L116)

Draws debug text on screen with specified size
Uses built-in debug font

### [static dbgLine(a, b) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L119)

Draws a debug line between two vector points

### [static dbgRect(fromX, fromY, toX, toY) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L124)

Draws a filled debug rectangle

### [static dbgSquare(centerX, centerY, size) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L137)

Draws a filled debug square centered at position

### [static dbgDisk(x, y, r, divs) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L144)

Draws a filled debug circle (disk)
divs controls the number of triangular segments

### [static dbgCircle(x, y, r, divs) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L163)

Draws a debug circle outline
divs controls the number of line segments

### [static dbgArc(x, y, r, angle, divs) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L181)

Draws a debug arc (partial circle outline)
angle is in radians, divs controls line segment count

### [static dbgPie(x, y, r, angle, divs) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L203)

Draws a filled debug pie/wedge shape
angle is in radians, divs controls triangle count

### [static dbgVertex(v) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L223)

Adds a vector point as a debug vertex

### [static sprite(spriteId, x, y) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L229)

Draws a sprite at position with default settings
Equivalent to: sprite(spriteId, x, y, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x00000000, spriteBottom)

### [static sprite(spriteId, x, y, z) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L234)

Draws a sprite at position with z-sorting

### [static sprite(spriteId, x, y, z, flags) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L239)

Draws a sprite at position with z-sorting and custom flags

### [static createGridSprite(imageId, columns, rows,  c, r) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L244)

Creates a sprite from a grid/sprite sheet by column and row

### [static createGridSprite(imageId, columns, rows,  idx) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L254)

Creates a sprite from a grid/sprite sheet by index
Index starts at 0 from top-left, going row by row

---
## [Class File](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L268)

File I/O operations

## API

### [static read(src)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L270)

Reads the contents of a file as a string

### [static write(text, dst)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L273)

Writes text content to a file

### [static exists(src)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L276)

Checks if a file exists at the given path

---
## [Class TouchData](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L280)

Data class for touch input information

---
## [Class Input](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L293)

Input handling for keyboard, mouse, gamepad, and touch

## API

### [static getAxis(axis)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L295)

Gets the current value of a gamepad axis (-1.0 to 1.0)

### [static getAxisOnce(axis, threshold)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L298)

Gets axis value once when it crosses threshold (prevents repeating)

### [static getButton(button)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L301)

Checks if a gamepad button is currently pressed

### [static getButtonOnce(button)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L304)

Checks if a gamepad button was just pressed (doesn't repeat while held)

### [static getKey(key)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L307)

Checks if a keyboard key is currently pressed

### [static getKeyOnce(key)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L310)

Checks if a keyboard key was just pressed (doesn't repeat while held)

### [static getMouse()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L313)

Gets mouse state information

### [static getMouseButton(button)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L316)

Checks if a mouse button is currently pressed

### [static getMouseButtonOnce(button)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L319)

Checks if a mouse button was just pressed (doesn't repeat while held)

### [static getMouseX()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L322)

Gets the current mouse X position in screen coordinates

### [static getMouseY()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L325)

Gets the current mouse Y position in screen coordinates

### [static getMouseWheel()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L328)

Gets the mouse wheel delta for this frame

### [static getNrTouches()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L331)

Gets the number of active touch points

### [static getTouchId(index)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L334)

Gets the unique ID for a touch at the given index

### [static getTouchX(index)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L337)

Gets the X position for a touch at the given index

### [static getTouchY(index)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L340)

Gets the Y position for a touch at the given index

### [static setPadVibration(lowRumble, highRumble, time)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L344)

Sets gamepad vibration motors (DualSense, Xbox controllers)
time is in milliseconds

### [static setPadLightbarColor(red, green, blue)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L348)

Sets the gamepad lightbar color (DualSense controller)
Colors are 0-255

### [static resetPadLightbarColor()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L351)

Resets gamepad lightbar to default color

### [static getTouchData() {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L354)

Gets all touch data as a list of TouchData objects

### [static getTouchData(index) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L362)

Gets touch data for a specific touch index

### [static getMousePosition() {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L367)

Gets the mouse position as a two-element list [x, y]

---
## [Class Audio](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L436)

Audio playback using FMOD

## API

### [static load(name, groupId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L439)

Loads a sound file into a group and returns a sound ID

### [static play(soundId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L442)

Plays a loaded sound and returns a channel ID

### [static getGroupVolume(groupId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L445)

Gets the volume level of a sound group (0.0 to 1.0)

### [static setGroupVolume(groupId, volume)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L448)

Sets the volume level of a sound group (0.0 to 1.0)

### [static getChannelVolume(channelId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L451)

Gets the volume level of a specific channel (0.0 to 1.0)

### [static setChannelVolume(channelId, volume)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L454)

Sets the volume level of a specific channel (0.0 to 1.0)

### [static getBusVolume(busName)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L457)

Gets the volume level of an FMOD bus by name (0.0 to 1.0)

### [static setBusVolume(busName, volume)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L460)

Sets the volume level of an FMOD bus by name (0.0 to 1.0)

### [static loadBank(bankId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L463)

Loads an FMOD sound bank

### [static unloadBank(bankId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L466)

Unloads an FMOD sound bank

### [static startEvent(eventName)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L469)

Starts an FMOD event and returns an event instance ID

### [static setParameterNumber(eventId, paramName, newValue)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L472)

Sets a numeric parameter on an FMOD event instance

### [static setParameterLabel(eventId, paramName, newValue)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L475)

Sets a labeled parameter on an FMOD event instance

### [static load(path)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L483)

Load an audio file and return an audio id

### [static play(audioId, volume)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L486)

Play a loaded audio file with specified volume (0.0 to 1.0)

### [static play(audioId) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L489)

Play a loaded audio file with default volume (1.0)

### [static setVolume(channelId, volume)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L494)

Set the volume of a playing channel (0.0 to 1.0)

### [static getVolume(channelId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L497)

Get the volume of a playing channel

### [static stop(channelId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L500)

Stop a playing channel

### [static stopAll()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L503)

Stop all playing channels

### [static isPlaying(channelId)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L506)

Check if a channel is currently playing

### [static getNumber(name) { getNumber(name, game) }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L511)

Gets a number value from the game scope

### [static getColor(name)  { getColor(name, game) }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L514)

Gets a color value from the game scope

### [static getBool(name)  { getBool(name, game) }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L517)

Gets a boolean value from the game scope

### [static getNumber(name, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L520)

Gets a number value from a specific data scope

### [static getColor(name, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L523)

Gets a color value from a specific data scope

### [static getBool(name, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L526)

Gets a boolean value from a specific data scope

### [static getString(name, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L529)

Gets a string value from a specific data scope

### [static setNumber(name, value, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L532)

Sets a number value in a specific data scope

### [static setColor(name, value, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L535)

Sets a color value in a specific data scope

### [static setBool(name, value, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L538)

Sets a boolean value in a specific data scope

### [static setString(name, value, type)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L541)

Sets a string value in a specific data scope

---
## [Class Device](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L550)

Platform and device information

## API

### [static getPlatform()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L553)

Gets the current platform identifier

### [static canClose()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L556)

Checks if the application can be closed (always false on consoles)

### [static requestClose()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L559)

Requests the application to close

---
## [Class Profiler](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L568)

CPU profiling utilities

## API

### [static begin(name)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L570)

Begins a named profiler section

### [static end(name)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs.wren#L573)

Ends a named profiler section
