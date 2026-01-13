import "xs_math" for Math, Bits, Vec2
import "xs_tools" for Tools
import "xs" for Inspector

// Module-level temporary storage for reflection (used by Entity.inspect)
var ReflectionTarget = null
var ReflectionValue = null
var SelectedEntityIndex = -1

/// Base class for components that can be added to entities
/// Components should inherit from this class and override initialize(), update(), and/or finalize()
/// Each entity can only have one component of each type
class Component {

    /// Creates a new component
    /// IMPORTANT: Always call super() first when creating a new component subclass constructor
    /// Note: Other components might not be available yet - use initialize() to query them
    construct new() {
        _owner = null
        _initialized = false
        _enabled = true
    }

    /// Called right before the first update
    /// This is the ideal place to query and cache references to other components on the same entity
    /// Example: _transform = owner.get(Transform)
    initialize() {}

    /// Called when the component/entity is deleted
    /// Clean up any references to other entities and components by setting them to null
    /// This prevents memory leaks from circular references
    finalize() {}

    /// Called once per frame with delta time in seconds
    /// Put your game logic here - this is only called when the component is enabled
    /// dt is typically 1/60 (0.0166...) for 60 FPS
    update(dt) {}

    /// Gets the Entity object that owns this component
    owner { _owner }

    /// Sets the owner (used internally by Entity)
    owner=(o) { _owner = o }

    /// Checks if the component is enabled
    /// If not enabled, the update() function will not be called
    enabled { _enabled }

    /// Sets the enabled state of the component
    enabled=(e) { _enabled = e }

    /// Gets the initialized state (used internally by Entity)
    initialized_ { _initialized }

    /// Sets the initialized state (used internally by Entity)
    initialized_=(i) { _initialized = i }
}

/// Represents a game object that can contain multiple components
/// Entities are managed by the Entity-Component system and should be created with Entity.new()
/// Call Entity.initialize() and Entity.update(dt) in your game's initialize() and update() methods
class Entity {

    /// Creates a new entity that will be visible to the rest of the game in the next update
    /// The entity won't appear in Entity.entities until the next frame
    construct new() {
        _components = {}
        _deleted = false
        _name = ""
        _tag = 0
        _compDeleteQueue = []
        __addQueue.add(this)
    }

    /// Adds a component to the entity
    /// The component must be a subclass of Component
    /// If a component of the same type already exists, it will be finalized and replaced
    /// Components are initialized and updated in the order they were added
    add(component) {
        var c = get(component.type)
        if(c != null) {
            c.finalize()
            // remove from the delete list (if it was there)
            Tools.removeFromList(_compDeleteQueue, c.type)
            // _compDeleteQueue.remove(c.type)
        }

        // TODO: Check if it already it has an owner
        component.owner = this
        _components[component.type] = component

        return component
    }

    /// Gets a component of the matching type, or null if not found
    /// Example: var transform = entity.get(Transform)
    get(type) {
        // TODO: Check if v is a type
        if (_components.containsKey(type)) {
            return _components[type]            
        }
        for(v in _components.values) {
            if(v is type) {
                return v    
            }
        }
        return null
    }

    /// Marks a component for removal at the end of the current update frame
    /// The component's finalize() method will be called before removal
    remove(type) {
        // TODO: Make the compoenent aware that it is being removed
        // by setting its owner to null
        if (_components.containsKey(type)) {
            _compDeleteQueue.add(type)
        } else {
            for(v in _components.values) {
                if(v is type) {
                    _compDeleteQueue.add(v.type)
                }
            }   
        }        
    }

    /// Gets all components attached to this entity
    components { _components.values }

    /// Checks if the entity is marked for deletion
    /// If true, you should set any references to this entity to null to avoid accessing deleted entities
    deleted { _deleted }

    /// Marks the entity for removal at the end of the current update frame
    /// All components will have their finalize() methods called before the entity is removed
    delete() { _deleted = true }

    /// Gets the name of the entity (useful for debugging)
    name { _name }
    /// Sets the name of the entity
    name=(n){ _name = n }

    /// Gets the tag (used as a bitflag when filtering entities)
    tag { _tag }
    /// Sets the tag
    tag=(t) { _tag = t }

    /// Sets the enabled state of all components
    enabled=(e) {
        for(c in _components.values) {
            c.enabled = e
        }
    }

    /// Initializes the entity system - MUST be called once at game startup
    /// Call this from your game's initialize() method before creating any entities
    /// Example: Entity.initialize()
    static initialize() {
        __entities = []
        __addQueue = []
    }

    /// Updates all entities and their components - MUST be called every frame
    /// Call this from your game's update(dt) method
    /// Handles adding new entities, removing deleted ones, and updating all component logic
    /// Example: Entity.update(dt)
    static update(dt) {
        for (e in __entities) {
            e.removeDeletedComponents_()
        }

        for(a in __addQueue) {
            __entities.add(a)
        }
        __addQueue.clear()

        for (e in __entities) {
            for(c in e.components) {
                if(!c.initialized_) {
                    c.initialize()
                    c.initialized_ = true
                }
                if(c.enabled) {
                    c.update(dt)
                }
            }
        }

        var i = 0
        while(i < __entities.count) {
            var e = __entities[i]
            if(e.deleted) {
                for(c in e.components) {
                    c.finalize()
                }
                __entities.removeAt(i)
            } else {
                i = i + 1
            }
        }
    }

    /// Gets all entities where the tag matches exactly with the given tag (using bitwise AND)
    /// Use this when you need entities with ALL specified tag bits set
    static withTag(tag) {
        var found = []
        for (e in __entities) {
                if(Bits.checkBitFlag(e.tag, tag)) {
                found.add(e)
            }
        }
        return found
    }

    /// Gets all entities where the tag has ANY bit overlap with the given tag
    /// Use this when you need entities with at least one matching tag bit
    static withTagOverlap(tag) {
        var found = []
        for (e in __entities) {
                if(Bits.checkBitFlagOverlap(e.tag, tag)) {
                found.add(e)
            }
        }
        return found
    }

    /// Gets all entities where the tag does not have bit overlap with the given tag
    static withoutTagOverlap(tag) {
        var found = []
        for (e in __entities) {
                if(!Bits.checkBitFlagOverlap(e.tag, tag)) {
                found.add(e)
            }
        }
        return found
    }

    /// Sets the enabled state for all entities with matching tag overlap
    static setEnabled(tag, enabled) {
        for (e in __entities) {
            if(Bits.checkBitFlagOverlap(e.tag, tag)) {
                for(c in e.components) {
                    c.enabled = enabled
                }
            }
        }
    }

    /// Gets all entities active in the system
    static entities { __entities }

    /// Returns a string representation of this entity
    toString {
        var s = "{ Name: %(name) Tag: %(tag)"
            for(c in _components) {
                s = s + "     %(c)"
            }
            s = s + "  }"
        return s
    }

    /// Prints a formatted list of all entities and their components (for debugging)
    static print() {
        System.print("<<<<<<<<<< ecs stats >>>>>>>>>>")
        System.print("Active: %(__entities.count)")
        var i = 0
        for (e in __entities) {
            System.print("%(i) { Name: %(e.name) Tag:%(e.tag)")
            for(c in e.components) {
                System.print("     %(c.toString)")
            }
            System.print("  }")
            i = i + 1
        }
        System.print("<<<<<<<<<<<<< end >>>>>>>>>>>>>")
    }

    /// Displays entity inspector UI (called from C++ inspector)
    static inspect() {
        if (__entities.count == 0) {
            Inspector.text("No entities in scene")
            return
        }

        // Top panel: Entity List (full width, fixed height, with border)
        Inspector.beginChild("EntityList", 0, 150, true)
        
        Inspector.text("ENTITIES")
        Inspector.separator()
        Inspector.spacing()

        var i = 0
        for (entity in __entities) {
            var entityLabel = entity.name.isEmpty ? "Entity %(i)" : entity.name
            var uniqueLabel = "%(entityLabel)##entity_%(i)"
            var isSelected = (i == SelectedEntityIndex)

            if (Inspector.selectable(uniqueLabel, isSelected)) {
                SelectedEntityIndex = i
            }

            i = i + 1
        }

        Inspector.endChild()

        // Bottom panel: Selected Entity Inspector (full width, remaining height, with border)
        Inspector.beginChild("EntityInspector", 0, 0, true)

        Inspector.separator()

        if (SelectedEntityIndex >= 0 && SelectedEntityIndex < __entities.count) {
            var selectedEntity = __entities[SelectedEntityIndex]
            var entityLabel = selectedEntity.name.isEmpty ? "Entity %(SelectedEntityIndex)" : selectedEntity.name            

            Inspector.text("%(entityLabel)")
            Inspector.text("Tag: %(selectedEntity.tag)")
            
            Inspector.spacing()

            // Component inspector
            if (selectedEntity.components.count > 0) {
                var compIndex = 0
                for (component in selectedEntity.components) {
                    inspectComponent_(component, compIndex)
                    Inspector.spacing()
                    compIndex = compIndex + 1
                }
            } else {
                Inspector.text("No components")
            }
        } else {
            Inspector.text("Select an entity to inspect")
        }

        Inspector.endChild()
    }

    /// Inspects a single component, showing its properties via attributes
    static inspectComponent_(component, compIndex) {
        var componentName = component.type.name
        var uniqueLabel = "%(componentName)##comp_%(compIndex)"

        if (Inspector.collapsingHeader(uniqueLabel)) {
            Inspector.spacing()

            // Get inspectable properties via attributes
            var props = getInspectableProperties_(component)

            if (props.count > 0) {
                for (prop in props) {
                    var propName = prop["name"]
                    var metadata = prop["metadata"]
                    var hasSetter = hasSetter_(component, propName)

                    // Get the property value
                    var value = getPropertyValue_(component, propName)

                    // Display editable or read-only property
                    if (hasSetter) {
                        var newValue = displayEditableProperty_(componentName, propName, value, metadata)
                        if (newValue != value) {
                            setPropertyValue_(component, propName, newValue)
                        }
                    } else {
                        var valueStr = formatValue_(value)
                        var typeHint = metadata.containsKey("type") ? " (%(metadata["type"]))" : ""
                        Inspector.text("%(propName): %(valueStr)%(typeHint)")
                    }
                }
            } else {
                Inspector.text("No inspectable properties")
            }
        }
    }

    /// Displays an editable property and returns the new value
    static displayEditableProperty_(componentName, propName, value, metadata) {
        var type = metadata.containsKey("type") ? metadata["type"] : "unknown"
        var uniqueId = "##%(componentName)_%(propName)"

        if (type == "angle" || type == "number") {
            if (value is Num) {
                return Inspector.dragFloat("%(propName)%(uniqueId)", value)
            }
        } else if (value is Vec2) {
            return Inspector.dragFloat2("%(propName)%(uniqueId)", value)
        } else if (value is Bool) {
            return Inspector.checkbox("%(propName)%(uniqueId)", value)
        } else if (value is Num) {
            return Inspector.dragFloat("%(propName)%(uniqueId)", value)
        }

        // Fallback: read-only display
        var valueStr = formatValue_(value)
        Inspector.text("%(propName): %(valueStr) (%(type))")
        return value
    }

    /// Gets all inspectable properties from a component using attributes
    static getInspectableProperties_(component) {
        var props = []
        var cls = component.type
        var attrs = cls.attributes

        if (attrs == null || attrs.methods == null) {
            return props
        }

        // Find all methods with #!inspect attribute
        for (methodSig in attrs.methods.keys) {
            var methodAttrMap = attrs.methods[methodSig]

            for (groupKey in methodAttrMap.keys) {
                var attrMap = methodAttrMap[groupKey]

                var hasInspect = (groupKey == "inspect")
                var metadata = {}

                for (attrName in attrMap.keys) {
                    var attrValues = attrMap[attrName]
                    if (attrName == "inspect") {
                        hasInspect = true
                    }
                    if (attrValues.count > 0) {
                        metadata[attrName] = attrValues[0]
                    }
                }

                if (hasInspect) {
                    // Extract property name from signature
                    var propName = methodSig
                    if (methodSig.endsWith("()")) {
                        propName = methodSig[0...-2]
                    }
                    // Skip setters
                    if (!methodSig.contains("=(")) {
                        props.add({
                            "name": propName,
                            "metadata": metadata
                        })
                    }
                    break
                }
            }
        }

        return props
    }

    /// Gets a property value from a component using reflection
    static getPropertyValue_(component, propName) {
        System.print("Getting property %(propName) from %(component.type.name)")
        import "meta" for Meta

        // Use module-level temp variables for eval
        ReflectionTarget = component
        var code = "ReflectionValue = ReflectionTarget.%(propName)"

        var fiber = Fiber.new { Meta.eval(code) }
        var result = fiber.try()

        if (fiber.error != null) {
            return "<error>"
        }

        var value = ReflectionValue
        ReflectionTarget = null
        ReflectionValue = null

        return value
    }

    /// Sets a property value on a component using reflection
    static setPropertyValue_(component, propName, value) {
        import "meta" for Meta

        // Use module-level temp variables for eval
        ReflectionTarget = component
        ReflectionValue = value

        var code = "ReflectionTarget.%(propName) = ReflectionValue"

        var fiber = Fiber.new { Meta.eval(code) }
        var result = fiber.try()

        ReflectionTarget = null
        ReflectionValue = null

        if (fiber.error != null) {
            System.print("Error setting %(propName): %(fiber.error)")
            return false
        }

        return true
    }

    /// Checks if a component has a setter for a property
    static hasSetter_(component, propName) {
        var cls = component.type
        var attrs = cls.attributes

        if (attrs == null || attrs.methods == null) {
            return false
        }

        // Check for setter signature: "propName=(_)"
        var setterSig = "%(propName)=(_)"
        return attrs.methods.containsKey(setterSig)
    }

    /// Formats a value for display
    static formatValue_(value) {
        if (value is List) {
            return "[%(value.join(", "))]"
        } else if (value is Num) {
            if (value == value.floor) {
                return value.toString
            } else {
                return value.toString
            }
        } else if (value is String) {
            return "\"%(value)\""
        } else if (value is Bool) {
            return value.toString
        } else {
            return value.toString
        }
    }

    /// Removes components marked for deletion (used internally)
    removeDeletedComponents_() {
        for(c in _compDeleteQueue) {
            _components[c].finalize()
            _components.remove(c)
        }
        _compDeleteQueue.clear()
    }
}
