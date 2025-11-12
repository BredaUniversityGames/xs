---
layout: default
title: Containers
parent: API Reference
nav_order: 6
---

<!-- file: assets/modules/xs_containers.wren -->
<!-- documentation automatically generated using domepunk/tools/doc -->
---
## [Class Grid](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L2)

Logical representation of a (game) grid

## API

### [width { _width }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L17)

The number of columns in the grid.

### [height { _height }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L20)

The number of rows in the grid.

### [swapValues(x1, y1, x2, y2) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L23)

Swaps the values of two given grid cells.

### [valid(x, y) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L36)

Checks if a given cell position exists in the grid.

### [[x, y] {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L41)

Returns the value stored at the given grid cell.

### [[x, y]=(v) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L46)

Assigns a given value to a given grid cell.

### [toString {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L51)

Constructs a string representation of this grid.

---
## [Class SparseGrid](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L63)

Logical representation of a grid with many empty spaces

## API

### [static makeId(x, y) { x << 16 | y }  // |](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L70)

Creates a unique identifier for a given cell position.

### [has(x, y) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L73)

Checks if a given cell position exists in the grid.

### [remove(x, y) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L79)

Removes the value stored at the given grid cell.

### [[x, y] {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L85)

Returns the value stored at the given grid cell.

### [[x, y]=(v) {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L96)

Assigns a given value to a given grid cell.

### [clear() {](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L102)

Clears the grid.

### [values { _grid.values }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L107)

Returns the values stored in the grid.

---
## [Class Queue](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L112)

First-in-first-out (FIFO) data structure

---
## [Class Dequeue](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L134)

Last-in-fist-out (LIFO data structure)

---
## [Class RingBuffer](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_containers.wren#L156)

A simple ring buffer implementation.
