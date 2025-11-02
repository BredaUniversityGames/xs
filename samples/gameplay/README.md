# Survivors-like Game Sample

A simple survivors-like action game demonstrating the ECS (Entity Component System) functionality of the xs engine.

## Game Description

This is a minimalist survivors-like game where:
- The player controls a white triangle that can move around the screen
- Enemies (smaller triangles) spawn at the edges and chase the player
- Enemies use obstacle avoidance to navigate around obstacles
- The player automatically shoots bullets in the direction they're facing
- Enemies take damage and are destroyed when hit by bullets
- Enemies drop collectible pickups when defeated
- The player takes damage when colliding with enemies
- Score is increased by collecting pickups
- Static obstacles (white discs) block bullets and influence enemy movement

## Controls

- **Left Stick / WASD**: Move player
- **Right Stick / Mouse**: Aim direction
- Shooting is automatic

## Architecture

The game follows an ECS architecture with the following components:

### Components
- **Transform**: Position and rotation
- **Body**: Size and velocity for physics
- **Sprite**: Visual representation
- **Health**: Hit points and damage system with optional pickup drops
- **Player**: Player movement (gamepad/keyboard/mouse) and shooting logic
- **Enemy**: Enemy AI that chases the player with obstacle avoidance
- **Bullet**: Projectile that damages enemies
- **Pickup**: Collectible that increases score
- **Shadow**: Renders shadows behind sprites
- **EnemySpawner**: Spawns enemies at intervals
- **CollisionSystem**: Handles all collision types (bullet-enemy, bullet-obstacle, player-enemy, player-pickup)

### Entity Types
- **Player**: Triangle that moves and shoots
- **Enemy**: Triangle that chases player and drops pickups
- **Bullet**: Small projectile
- **Obstacle**: Static disc objects that block bullets
- **Pickup**: Small collectible disc dropped by enemies
- **Spawner**: Invisible entity that spawns enemies
- **CollisionSystem**: Invisible entity that checks collisions

## Configuration

All game parameters can be adjusted in `game.json`:
- Player speed, health, shoot rate, size
- Enemy speed, health, damage, spawn rate, size, avoidance radius and strength
- Bullet speed, damage, lifetime, size
- Pickup value, lifetime, size
- World dimensions
- Colors for player, enemies, bullets, obstacles, pickups

## Assets

Simple placeholder graphics are generated using the `assets/generate_assets.py` script:
- White triangles for player and enemies
- White discs for obstacles
- White ellipse for bullets
- White discs (small) for pickups

All graphics use a simple white color that can be tinted via the configuration.
