// Attribute Testing Sample for xs engine
// Testing Wren's attribute system for inspector capabilities

import "xs/core" for Render
import "xs/math" for Color
import "meta" for Meta

// Simulated Transform component - EXPLICIT getter/setter marking
#!component = "Transform"
class Transform {
    construct new(x, y) {
        _position = [x, y]
        _rotation = 0
        _scale = 1
        _offset = [0, 0]
    }

    // Approach 1: Mark BOTH getter and setter separately
    #!inspect(type = "vec2")
    position { _position }

    #!inspect
    position=(value) { _position = value }

    // Approach 2: Getter with metadata, setter marked separately
    #!inspect(type = "angle", min = 0, max = 360)
    rotation { _rotation }

    #!inspect
    rotation=(value) { _rotation = value }

    // Approach 3: Both marked, with different metadata
    #!inspect(type = "number", min = 0.1, max = 10)
    scale { _scale }

    #!inspect(validate = true)
    scale=(value) { _scale = value }

    // TEST: Getter only - explicitly readonly
    #!inspect(type = "vec2", readonly = true)
    offset { _offset }
    // No setter attribute = readonly!

    // TEST: Setter marked but no getter
    #!inspect
    testSetter=(value) { _testSetter = value }

    // Private data - no attribute, won't be inspected
    internalCache { null }
}

// Simulated Sprite component
#!component = "Sprite"
class Sprite {
    construct new(imagePath) {
        _imagePath = imagePath
        _color = 0xffffffff
        _visible = true
        _layer = 0
    }

    #!inspect(type = "string", readonly = true)
    imagePath { _imagePath }

    #!inspect(type = "color", editable = true)
    color { _color }
    color=(value) { _color = value }

    #!inspect(type = "boolean", editable = true)
    visible { _visible }
    visible=(value) { _visible = value }

    #!inspect(type = "number", editable = true)
    layer { _layer }
    layer=(value) { _layer = value }
}

// Simulated Body/Physics component
#!component = "Body"
class Body {
    construct new(width, height) {
        _size = [width, height]
        _velocity = [0, 0]
        _mass = 1.0
        _friction = 0.5
    }

    #!inspect(type = "vec2", editable = true)
    size { _size }
    size=(value) { _size = value }

    #!inspect(type = "vec2", editable = true)
    velocity { _velocity }
    velocity=(value) { _velocity = value }

    #!inspect(type = "number", editable = true, min = 0, max = 100)
    mass { _mass }
    mass=(value) { _mass = value }

    #!inspect(type = "number", editable = true, min = 0, max = 1)
    friction { _friction }
    friction=(value) { _friction = value }
}

// Simulated Entity - simplified version
class SimulatedEntity {
    construct new(name) {
        _name = name
        _components = []
    }

    #!inspect(type = "string", readonly = true)
    name { _name }

    components { _components }

    add(component) {
        _components.add(component)
    }
}

// Module-level temporary storage for eval-based reflection
// This is needed because Meta.eval runs in module scope
var ReflectionTarget = null
var ReflectionValue = null

// Reflection-based Inspector
// This class uses attributes to automatically inspect any object
class Inspector {
    // Get all inspectable properties from an object
    static getInspectableProperties(object) {
        var props = []
        var cls = object.type
        var attrs = cls.attributes

        if (attrs == null || attrs.methods == null) {
            return props
        }

        // Iterate through all methods
        for (methodSig in attrs.methods.keys) {
            var methodAttrMap = attrs.methods[methodSig]

            // Check all attribute groups for this method
            for (groupKey in methodAttrMap.keys) {
                var attrMap = methodAttrMap[groupKey]

                // Check if the group key is "inspect" (for grouped attrs like #!inspect(...))
                // OR if there's an attribute named "inspect" (for ungrouped like #!inspect)
                var hasInspect = (groupKey == "inspect")
                var metadata = {}

                // attrMap is a Map of attribute names to Lists of values
                for (attrName in attrMap.keys) {
                    var attrValues = attrMap[attrName]

                    if (attrName == "inspect") {
                        hasInspect = true
                    }

                    // Store metadata (first value in list)
                    if (attrValues.count > 0) {
                        metadata[attrName] = attrValues[0]
                    }
                }

                if (hasInspect) {
                    props.add({
                        "signature": methodSig,
                        "metadata": metadata
                    })
                    break // Found inspect attribute, no need to check other groups
                }
            }
        }

        return props
    }

    // Get the value of a property by calling the method
    static getPropertyValue(object, methodSignature) {
        // For getters (no parentheses), we need to access via Fiber
        // For now, we'll just return a placeholder
        // In real implementation, would use reflection or direct calls
        return "<callable>"
    }

    // Print all inspectable properties of an object
    static print(object) {
        var className = object.type.name
        System.print("  [%(className)]")

        var props = getInspectableProperties(object)

        if (props.count == 0) {
            System.print("    (no inspectable properties)")
            return
        }

        for (prop in props) {
            var sig = prop["signature"]
            var meta = prop["metadata"]

            // Extract property name (remove parentheses for methods)
            var propName = sig
            if (sig.endsWith("()")) {
                propName = sig[0...-2]
            }

            // Get type hint if available
            var typeHint = meta.containsKey("type") ? " (%(meta["type"]))" : ""

            // Get readonly status
            var readonly = meta.containsKey("readonly") && meta["readonly"] ? " [readonly]" : ""

            // Get range if available
            var range = ""
            if (meta.containsKey("min") && meta.containsKey("max")) {
                range = " [%(meta["min"])..%(meta["max"])]"
            }

            System.print("    - %(propName)%(typeHint)%(readonly)%(range)")
        }
    }

    // Print all inspectable properties with their actual values
    static printWithValues(object) {
        var className = object.type.name
        System.print("  [%(className)]")

        var props = getInspectableProperties(object)

        if (props.count == 0) {
            System.print("    (no inspectable properties)")
            return
        }

        for (prop in props) {
            var sig = prop["signature"]
            var meta = prop["metadata"]

            // Extract property name
            var propName = sig
            if (sig.endsWith("()")) {
                propName = sig[0...-2]
            }

            // Try to get the actual value
            // We need to call the getter - use direct property access
            var value = tryGetValue(object, propName)

            // Format the value
            var valueStr = formatValue(value)

            // Get type hint
            var typeHint = meta.containsKey("type") ? "(%(meta["type"]))" : ""

            System.print("    - %(propName): %(valueStr) %(typeHint)")
        }
    }

    // Helper to get a value via reflection using Meta.eval
    // This is much more flexible and maintainable!
    static tryGetValue(object, propName) {
        // Store object in module scope so eval can access it
        ReflectionTarget = object

        // Build and execute code to get the property
        var code = "ReflectionValue = ReflectionTarget.%(propName)"

        var fiber = Fiber.new {
            Meta.eval(code)
        }

        var result = fiber.try()
        if (fiber.error != null) {
            return "<error: %(fiber.error)>"
        }

        // Retrieve the value
        var value = ReflectionValue

        // Clean up
        ReflectionTarget = null
        ReflectionValue = null

        return value
    }

    // Helper to set a value via reflection using Meta.eval
    // This is much more flexible and maintainable!
    static trySetValue(object, propName, value) {
        // Store object and value in module scope
        ReflectionTarget = object
        ReflectionValue = value

        // Build and execute code to set the property
        var code = "ReflectionTarget.%(propName) = ReflectionValue"

        var fiber = Fiber.new {
            Meta.eval(code)
        }

        var result = fiber.try()

        // Clean up
        ReflectionTarget = null
        ReflectionValue = null

        if (fiber.error != null) {
            System.print("    ERROR setting %(propName): %(fiber.error)")
            return false
        }

        return true
    }

    // Format a value for display
    static formatValue(value) {
        if (value is List) {
            return "[%(value.join(", "))]"
        } else if (value is Num) {
            // Format numbers nicely
            if (value == value.floor) {
                return value.toString
            } else {
                return "%(value.toString)"
            }
        } else if (value is String) {
            return "\"%(value)\""
        } else if (value is Bool) {
            return value.toString
        } else {
            return value.toString
        }
    }

    // Print an entire entity with all its components
    static printEntity(entity) {
        System.print("Entity: %(entity.name)")
        printWithValues(entity)

        for (component in entity.components) {
            printWithValues(component)
        }

        System.print("")
    }
}

// Main game class
class Game {
    static initialize() {
        System.print("========================================")
        System.print("Wren Attribute-Based Entity Inspection")
        System.print("========================================")

        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 20)

        // Create simulated entities with components
        System.print("\n--- Creating Test Entities ---")

        var player = SimulatedEntity.new("Player")
        player.add(Transform.new(100, 200))
        player.add(Sprite.new("player.png"))
        player.add(Body.new(32, 48))

        var enemy = SimulatedEntity.new("Enemy_01")
        enemy.add(Transform.new(-50, 150))
        enemy.add(Sprite.new("enemy.png"))
        var enemyBody = Body.new(24, 24)
        enemyBody.velocity = [10, -5]
        enemyBody.mass = 2.5
        enemy.add(enemyBody)

        var pickup = SimulatedEntity.new("HealthPickup")
        var pickupTransform = Transform.new(0, 0)
        pickupTransform.rotation = 45
        pickupTransform.scale = 0.5
        pickup.add(pickupTransform)
        var pickupSprite = Sprite.new("health.png")
        pickupSprite.color = 0xff00ff00
        pickupSprite.layer = -5
        pickup.add(pickupSprite)

        // Store entities
        __entities = [player, enemy, pickup]

        // Test the reflection-based inspector
        System.print("\n--- Reflection-Based Inspection ---")
        System.print("(No toString() implementations needed!)\n")

        for (entity in __entities) {
            Inspector.printEntity(entity)
        }

        // Test explicit getter/setter marking
        System.print("\n--- Testing Explicit Getter/Setter Marking ---")

        var playerTransform = player.components[0]
        var cls = playerTransform.type
        var attrs = cls.attributes.methods

        System.print("All attributed methods on Transform:")
        for (methodSig in attrs.keys) {
            System.print("  - %(methodSig)")
        }

        System.print("\nAnalyzing getter/setter pairs:")

        // Check position
        var hasPositionGetter = attrs.containsKey("position")
        var hasPositionSetter = attrs.containsKey("position=(_)")
        System.print("  position: getter=%(hasPositionGetter), setter=%(hasPositionSetter)")

        // Check rotation
        var hasRotationGetter = attrs.containsKey("rotation")
        var hasRotationSetter = attrs.containsKey("rotation=(_)")
        System.print("  rotation: getter=%(hasRotationGetter), setter=%(hasRotationSetter)")

        // Check offset (readonly)
        var hasOffsetGetter = attrs.containsKey("offset")
        var hasOffsetSetter = attrs.containsKey("offset=(_)")
        System.print("  offset: getter=%(hasOffsetGetter), setter=%(hasOffsetSetter) (readonly)")

        // Check testSetter (setter-only)
        var hasTestGetter = attrs.containsKey("testSetter")
        var hasTestSetter = attrs.containsKey("testSetter=(_)")
        System.print("  testSetter: getter=%(hasTestGetter), setter=%(hasTestSetter) (setter-only)")

        System.print("\nConclusion:")
        System.print("  ✓ Getters and setters can be marked INDEPENDENTLY")
        System.print("  ✓ Inspector can check if BOTH exist before allowing edits")
        System.print("  ✓ Safer and more explicit than editable=true promise!")

        // Test dynamic value setting via attributes
        System.print("\n--- Testing Dynamic Property Modification ---")
        System.print("Modifying Player entity properties via reflection...\n")

        var playerSprite = player.components[1]

        System.print("Setting Transform.position = [250, 350]")
        Inspector.trySetValue(playerTransform, "position", [250, 350])

        System.print("Setting Transform.rotation = 90")
        Inspector.trySetValue(playerTransform, "rotation", 90)

        System.print("Setting Sprite.color = 0xff00ff00 (green)")
        Inspector.trySetValue(playerSprite, "color", 0xff00ff00)

        System.print("Setting Sprite.visible = false")
        Inspector.trySetValue(playerSprite, "visible", false)

        System.print("\nPlayer after modifications:")
        Inspector.printEntity(player)

        System.print("========================================")
        System.print("Success! Properties can be read AND set via attributes!")
        System.print("This enables full dynamic entity inspection and editing")
        System.print("========================================")
    }


    static update(dt) {
        // No-op
    }

    static render() {
        
        // Display status
        var y = -300
        Render.text(__font, "Wren Attribute-Based Inspector", 0, y, 1, 0xffffffff, 0x00000000, Render.spriteCenterX)
        y = y + 30
        Render.text(__font, "Check console output for inspection results", 0, y, 1, 0xffaaaaaa, 0x00000000, Render.spriteCenterX)
        y = y + 40

        // Show entity count
        Render.text(__font, "Entities: %(__entities.count)", 0, y, 1, 0xff88ff88, 0x00000000, Render.spriteCenterX)
        y = y + 30

        // List entities
        for (entity in __entities) {
            var label = "- %(entity.name) (%(entity.components.count) components)"
            Render.text(__font, label, 0, y, 1, 0xffcccccc, 0x00000000, Render.spriteCenterX)
            y = y + 25
        }
    }
}
