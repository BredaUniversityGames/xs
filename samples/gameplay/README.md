# Survivors-like Game Sample

A simple survivors-like action game demonstrating the ECS (Entity Component System) functionality of the xs engine.

## Game Description

This is a minimalist survivors-like game where:
- The player controls a white triangle that can move around the screen
- Enemies (smaller triangles) spawn at the edges and chase the player
- The player automatically shoots bullets in the direction they're facing
- Enemies take damage and are destroyed when hit by bullets
- The player takes damage when colliding with enemies
- Score is based on survival time
- Static obstacles (white discs) are placed around the arena

## Controls

- **Left Stick / WASD**: Move player
- **Right Stick**: Aim direction
- Shooting is automatic

## Architecture

The game follows an ECS architecture with the following components:

### Components
- **Transform**: Position and rotation
- **Body**: Size and velocity for physics
- **Sprite**: Visual representation
- **Health**: Hit points and damage system
- **Player**: Player movement and shooting logic
- **Enemy**: Enemy AI that chases the player
- **Bullet**: Projectile that damages enemies
- **Shadow**: Renders shadows behind sprites
- **EnemySpawner**: Spawns enemies at intervals
- **CollisionSystem**: Handles bullet-enemy and player-enemy collisions

### Entity Types
- **Player**: Triangle that moves and shoots
- **Enemy**: Triangle that chases player
- **Bullet**: Small projectile
- **Obstacle**: Static disc objects
- **Spawner**: Invisible entity that spawns enemies
- **CollisionSystem**: Invisible entity that checks collisions

## Configuration

All game parameters can be adjusted in `game.json`:
- Player speed, health, shoot rate, size
- Enemy speed, health, damage, spawn rate, size
- Bullet speed, damage, lifetime, size
- World dimensions
- Colors for player, enemies, bullets, obstacles

## Assets

Simple placeholder graphics are generated using the `assets/generate_assets.py` script:
- White triangles for player and enemies
- White discs for obstacles
- White ellipse for bullets

All graphics use a simple white color that can be tinted via the configuration.
