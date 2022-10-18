---
layout: default
title: Entities and Components
nav_order: 2
parent: Optional
---

# Entities and Components

# Enity
Entry point into the Entity Component system, alowing you to create entities and add componenets to them. Entity itself manages the entities.  

## construct new() 
Creates a new entity, visible to the rest of the game in the next update.
## addComponent(component)
Adds component to the entity.
## getComponent(type)
Get a component of a matching type.
## getComponentSuper(type)
Get a component of a matching super class type.
Slow! Use getComponent(type) when possible
## deleteComponent(type)  
Will mark the component for removal at the end of the update      
## components 
Get all to components 
## deleted
Checks if the entity is marked for removal. Set reference to this entity to null if true, so the the GC can delete the object

## delete()
Will mark the entity for removal at the end of the update
## name
Components can have names. This makes debugguig much easier
## name=(n)
## tag
Generic tag. Used as a bitflag when getting entities of certain type.
## tag=(t)
  
## static init
Call from the init() function of you entry point (game class)
## static update(dt)
Call from the update() function of you entry point (game class). Updates the all entities and their compoenents. Add/removes entities and componenets.
## static entitiesWithTag(tag) 
Get all the entities where the tag matches (has a bit overlap) with a given tag.

## static entities
Access to all the entities active in the game.

## static print()
Does a formated print of all the entities and their componenets. Override the toString  property on the component to mkae it more informative

# Component
A base class for components that can be added to the entities.
## construct new()
Creates a new component. Make sure to call super() when inheriting from this class. Other components might still not be avaialbe on the owning entity.
## initilize()
Called right before the first update. Good place to query and cache other components.
## finalize()
Called when the component/entity is deleted. Set any references to other entities and components to null.
## update(dt)
Called once per update with delta time. Put your logic here.
## owner
The Entity object that owns this compoenent