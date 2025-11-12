---
layout: default
title: Shapes
parent: API Reference
nav_order: 5
---

<!-- file: assets/modules/xs_shapes.wren -->
<!-- documentation automatically generated using domepunk/tools/doc -->
## API

### [import "xs" for Render, File, Profiler](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L7)


xs_shapes
A module for rendering shapes and text. Made to work bridge the
xs engine and the svg Python tools.

### [static render() {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L237)

Render all shapes in the scene

### [/// Create a quad shape](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L268)

Filled shapes ////////////////////////////////////////////////

### [static disk(center, radius, segments, color) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L281)

Create a new disk shape (or any regular polygon in fact)

### [static rectangle(p0, p1, color) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L299)

Create a new rectangle shape

### [static fill(vertices, color) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L307)

Fill (convex for now) poly shape

### [var center = Vec2.new(0, 0)](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L309)

Find the center of the polygon

### [var shape = Shape.new()](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L316)

Create the shape

### [/// Create a new line shape](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L327)

Outlined shapes //////////////////////////////////////////////

### [static stroke(points, thickness, color) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L341)

Draw a line of points

### [var p_prev = points[(i - 1) % points.count]](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L348)

Use three points of the line to  average the normal

### [/// Create rounded polygon points](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_shapes.wren#L379)

Point generation ////////////////////////////////////////////
