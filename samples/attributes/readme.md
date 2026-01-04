# Wren Attributes Test Sample

This sample demonstrates Wren's attribute reflection capabilities for the xs engine.

## What Are Attributes?

Attributes are metadata annotations that can be attached to classes and methods in Wren. They're accessible at runtime and can be used for introspection, tooling, and dynamic behavior.

## Syntax

### Runtime Attributes
Use `#!` prefix to make attributes accessible at runtime (without `!`, they're compiled out):

```wren
// Class attributes
#!component = true
#!version = "1.0"
class MyComponent { }

// Method attributes - simple
#!inspect
position { _position }

// Method attributes - with value
#!inspect = true
#!readonly = true
getName() { _name }

// Method attributes - grouped
#!inspect(editable = true, type = "number", min = 0, max = 10)
scale { _scale }
```

## Runtime Access

### Class Attributes
```wren
var attrs = MyComponent.attributes.self
// Returns: {null: {component: [true], version: [1.0]}}
```

### Method Attributes
```wren
var methodAttrs = MyComponent.attributes.methods
// Returns a Map keyed by method signature:
// {
//   "position": {null: {inspect: [null]}},
//   "getName()": {null: {inspect: [true], readonly: [true]}},
//   "scale": {inspect: {editable: [true], type: [number], min: [0], max: [10]}}
// }
```

## Data Structure

Attributes are organized as:
- **attributes.self**: `Map<GroupKey, Map<AttrName, List<AttrValue>>>`
- **attributes.methods**: `Map<MethodSignature, Map<GroupKey, Map<AttrName, List<AttrValue>>>>`

Where:
- **GroupKey**: `null` for ungrouped attributes, or the group name for `#!group(...)`
- **AttrName**: The attribute name (e.g., `"inspect"`, `"type"`)
- **AttrValue**: Can be `null`, `Bool`, `Num`, or `String`
- **MethodSignature**: The full method signature (e.g., `"position"`, `"getName()"`, `"static getVersion()"`)

Values are stored in **Lists** because duplicate keys are allowed.

## Use Cases for Entity Inspection

Attributes enable:

1. **Automatic Property Discovery**: Mark getters with `#!inspect` to expose them in the inspector
2. **Property Metadata**: Add type hints, ranges, validation rules via grouped attributes
3. **Read-only Properties**: Mark with `#!readonly = true` to prevent editing
4. **Custom Serialization**: Use attributes to control which fields are saved/loaded
5. **UI Generation**: Build inspector UI dynamically based on attribute metadata

## Example: Inspector-Ready Component

```wren
#!component = "Transform"
#!icon = "move"
class Transform {
    construct new() {
        _position = [0, 0]
        _rotation = 0
    }

    #!inspect(type = "vec2", editable = true)
    position { _position }
    position=(v) { _position = v }

    #!inspect(type = "angle", min = 0, max = 360, editable = true)
    rotation { _rotation }
    rotation=(v) { _rotation = v }

    // Not marked with #!inspect - won't appear in inspector
    internalState { _somePrivateData }
}
```

Then in the inspector code:
```wren
static getInspectableProperties(component) {
    var props = []
    var attrs = component.type.attributes.methods

    for (methodSig in attrs.keys) {
        var methodAttrs = attrs[methodSig]

        // Check if method has #!inspect attribute
        for (groupKey in methodAttrs.keys) {
            var attrMap = methodAttrs[groupKey]
            if (attrMap.containsKey("inspect")) {
                props.add({
                    "name": methodSig,
                    "type": attrMap.containsKey("type") ? attrMap["type"][0] : "unknown",
                    "editable": attrMap.containsKey("editable") ? attrMap["editable"][0] : false
                })
            }
        }
    }

    return props
}
```

## Running This Sample

```bash
xs run samples/attributes
```

The sample will:
1. Test class and method attribute access
2. Display attribute data structures
3. Demonstrate dynamic property discovery
4. Show integration with xs inspector

Check the console output for detailed test results.
