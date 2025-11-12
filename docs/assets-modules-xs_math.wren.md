---
layout: default
title: Math & Vectors
parent: API Reference
nav_order: 2
---

<!-- file: assets/modules/xs_math.wren -->
<!-- documentation automatically generated using domepunk/tools/doc -->
## API

### [// Math tools](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_math.wren#L2)

////////////////////////////////////////////////////////////////////////////

### [import "random" for Random](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_math.wren#L5)

////////////////////////////////////////////////////////////////////////////

---
## [Class Math](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_math.wren#L8)

Mathematical utility functions

## API

### [static lerp(a, b, t) { (a * (1.0 - t)) + (b * t) }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_math.wren#L13)

Linear interpolation between two values
Returns a value between a and b based on parameter t (0.0 to 1.0)

### [static damp(a, b, lambda, dt) { lerp(a, b, 1.0 - (-lambda * dt).exp) }](https://github.com/BredaUniversityGames/xs/blob/main/assets/modules/xs_math.wren#L17)

Smooth damping interpolation using exponential decay
Useful for camera following and smooth movement
