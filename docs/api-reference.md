---
layout: default
title: API Reference
nav_order: 3
has_children: true
---

# <i class="fas fa-book"></i> API Reference

Complete API documentation for the **xs** game engine, auto-generated from Wren module files.

## Core Modules

The xs engine provides several modules for game development:

<div class="features">
  <div class="feature-card">
    <div class="feature-icon"><i class="fas fa-palette"></i></div>
    <h3>Core API</h3>
    <p>Rendering, input, audio, file I/O, and device APIs</p>
  </div>

  <div class="feature-card">
    <div class="feature-icon"><i class="fas fa-calculator"></i></div>
    <h3>Math & Vectors</h3>
    <p>2D vector math, colors, geometry, and bit operations</p>
  </div>

  <div class="feature-card">
    <div class="feature-icon"><i class="fas fa-cubes"></i></div>
    <h3>Entity-Component</h3>
    <p>Flexible EC system for organizing game logic</p>
  </div>

  <div class="feature-card">
    <div class="feature-icon"><i class="fas fa-shapes"></i></div>
    <h3>Shapes & Containers</h3>
    <p>Shape utilities and data structures</p>
  </div>
</div>

## Usage

Import modules in your Wren code like this:

```wren
// Import specific classes from a module
import "xs" for Render, Input, Audio

// Import math utilities
import "xs_math" for Vec2, Math, Color

// Import entity-component system
import "xs_ec" for Entity, Component
```

## Module Overview

Each module provides classes and functions for specific functionality:

- **Core API** (`xs`) - Main engine features (Render, Input, Audio, File, Device, Data)
- **Math** (`xs_math`) - Math utilities, Vec2, Color, Geom classes
- **Entity-Component** (`xs_ec`) - Entity and Component base classes
- **Components** (`xs_components`) - Pre-built component implementations
- **Shapes** (`xs_shapes`) - Shape drawing utilities
- **Containers** (`xs_containers`) - Data structure utilities
- **Tools** (`xs_tools`) - General utility functions
- **Assert** (`assert`) - Assertion utilities for testing

Browse the pages in this section to explore the full API documentation.
