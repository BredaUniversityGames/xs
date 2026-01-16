// JSON API Test Sample for xs engine
// Testing the C++ JSON load/parse/save/stringify functions

import "xs/core" for Render, Json, File

class Game {
    static initialize() {
        System.print("========================================")
        System.print("JSON API Test (C++ Implementation)")
        System.print("========================================")

        __font = Render.loadFont("[shared]/fonts/selawk.ttf", 20)
        __results = []
        __allPassed = true

        // Run all tests
        testLoad()
        testParse()
        testStringify()
        testSave()
        testRoundTrip()
        testEdgeCases()

        // Print summary
        System.print("\n========================================")
        if (__allPassed) {
            System.print("ALL TESTS PASSED!")
        } else {
            System.print("SOME TESTS FAILED - check output above")
        }
        System.print("========================================")
    }

    static testLoad() {
        System.print("\n--- Test: Json.load() ---")

        // Test loading existing file
        var data = Json.load("[game]/test_data.json")

        if (data == null) {
            fail("Failed to load test_data.json")
            return
        }

        // Verify structure
        pass("Loaded JSON file successfully")

        // Check string value
        if (data["name"] == "Test Entity") {
            pass("String value correct: %(data["name"])")
        } else {
            fail("String value incorrect: %(data["name"])")
        }

        // Check nested object
        var pos = data["position"]
        if (pos != null && pos["x"] == 100.5 && pos["y"] == 200) {
            pass("Nested object correct: x=%(pos["x"]), y=%(pos["y"])")
        } else {
            fail("Nested object incorrect")
        }

        // Check array
        var tags = data["tags"]
        if (tags != null && tags.count == 3 && tags[0] == "player") {
            pass("Array correct: %(tags)")
        } else {
            fail("Array incorrect")
        }

        // Check boolean
        var stats = data["stats"]
        if (stats != null && stats["isAlive"] == true) {
            pass("Boolean value correct: isAlive=%(stats["isAlive"])")
        } else {
            fail("Boolean value incorrect")
        }

        // Check null value
        if (data["nullValue"] == null) {
            pass("Null value correct")
        } else {
            fail("Null value incorrect: %(data["nullValue"])")
        }

        // Check array of objects
        var inventory = data["inventory"]
        if (inventory != null && inventory.count == 3 && inventory[0]["item"] == "sword") {
            pass("Array of objects correct: first item=%(inventory[0]["item"])")
        } else {
            fail("Array of objects incorrect")
        }

        // Test loading non-existent file
        var missing = Json.load("[game]/nonexistent.json")
        if (missing == null) {
            pass("Non-existent file returns null")
        } else {
            fail("Non-existent file should return null")
        }

        __loadedData = data
    }

    static testParse() {
        System.print("\n--- Test: Json.parse() ---")

        // Simple object
        var obj = Json.parse("{\"name\": \"test\", \"value\": 42}")
        if (obj != null && obj["name"] == "test" && obj["value"] == 42) {
            pass("Parse simple object: name=%(obj["name"]), value=%(obj["value"])")
        } else {
            fail("Parse simple object failed")
        }

        // Array
        var arr = Json.parse("[1, 2, 3, \"four\", true, null]")
        if (arr != null && arr.count == 6 && arr[0] == 1 && arr[3] == "four" && arr[4] == true && arr[5] == null) {
            pass("Parse array: %(arr)")
        } else {
            fail("Parse array failed")
        }

        // Nested structure
        var nested = Json.parse("{\"outer\": {\"inner\": [1, 2, 3]}}")
        if (nested != null && nested["outer"]["inner"][1] == 2) {
            pass("Parse nested structure: inner[1]=%(nested["outer"]["inner"][1])")
        } else {
            fail("Parse nested structure failed")
        }

        // Invalid JSON
        var invalid = Json.parse("{\"broken\": }")
        if (invalid == null) {
            pass("Invalid JSON returns null")
        } else {
            fail("Invalid JSON should return null")
        }

        // Empty object/array
        var emptyObj = Json.parse("{}")
        var emptyArr = Json.parse("[]")
        if (emptyObj != null && emptyArr != null) {
            pass("Empty object and array parse correctly")
        } else {
            fail("Empty object/array parse failed")
        }
    }

    static testStringify() {
        System.print("\n--- Test: Json.stringify() ---")

        // Simple values (note: all numbers become floats in JSON)
        var numStr = Json.stringify(42)
        if (numStr == "42" || numStr == "42.0") {
            pass("Stringify number: %(numStr)")
        } else {
            fail("Stringify number failed: %(numStr)")
        }

        var strStr = Json.stringify("hello")
        if (strStr == "\"hello\"") {
            pass("Stringify string: %(strStr)")
        } else {
            fail("Stringify string failed: %(strStr)")
        }

        var boolStr = Json.stringify(true)
        if (boolStr == "true") {
            pass("Stringify bool: %(boolStr)")
        } else {
            fail("Stringify bool failed: %(boolStr)")
        }

        var nullStr = Json.stringify(null)
        if (nullStr == "null") {
            pass("Stringify null: %(nullStr)")
        } else {
            fail("Stringify null failed: %(nullStr)")
        }

        // Array
        var arr = [1, "two", true, null]
        var arrStr = Json.stringify(arr)
        if (arrStr.contains("1") && arrStr.contains("\"two\"") && arrStr.contains("true") && arrStr.contains("null")) {
            pass("Stringify array contains expected values")
            System.print("    Array JSON: %(arrStr)")
        } else {
            fail("Stringify array failed: %(arrStr)")
        }

        // Nested array
        var nested = [[1, 2], [3, 4]]
        var nestedStr = Json.stringify(nested)
        if (nestedStr.contains("[") && nestedStr.contains("1") && nestedStr.contains("3")) {
            pass("Stringify nested array works")
            System.print("    Nested JSON: %(nestedStr)")
        } else {
            fail("Stringify nested array failed")
        }
    }

    static testSave() {
        System.print("\n--- Test: Json.save() ---")

        // Create test data
        var testData = [
            "item1",
            "item2",
            123,
            true,
            ["nested", "array"]
        ]

        // Save to file
        var success = Json.save("[save]/json_test_output.json", testData)
        if (success) {
            pass("Json.save() returned true")
        } else {
            fail("Json.save() returned false")
            return
        }

        // Verify by loading it back
        var loaded = Json.load("[save]/json_test_output.json")
        if (loaded != null && loaded.count == 5 && loaded[0] == "item1") {
            pass("Saved file loads correctly: %(loaded[0])")
        } else {
            fail("Saved file doesn't load correctly")
        }
    }

    static testRoundTrip() {
        System.print("\n--- Test: Round Trip (parse -> stringify -> parse) ---")

        var original = "{\"a\": 1, \"b\": [2, 3], \"c\": true}"

        // Parse
        var parsed = Json.parse(original)
        if (parsed == null) {
            fail("Initial parse failed")
            return
        }

        // Stringify
        var stringified = Json.stringify(parsed["b"]) // Just the array part
        System.print("    Stringified array: %(stringified)")

        // Parse again
        var reparsed = Json.parse(stringified)
        if (reparsed != null && reparsed.count == 2 && reparsed[0] == 2 && reparsed[1] == 3) {
            pass("Round trip preserves data: %(reparsed)")
        } else {
            fail("Round trip failed")
        }
    }

    static testEdgeCases() {
        System.print("\n--- Test: Edge Cases ---")

        // Large numbers
        var bigNum = Json.parse("999999999999")
        if (bigNum == 999999999999) {
            pass("Large number: %(bigNum)")
        } else {
            fail("Large number failed")
        }

        // Floating point
        var floatNum = Json.parse("3.14159265359")
        if ((floatNum - 3.14159265359).abs < 0.0000001) {
            pass("Float precision: %(floatNum)")
        } else {
            fail("Float precision failed: %(floatNum)")
        }

        // Unicode strings
        var unicode = Json.parse("{\"emoji\": \"Hello\"}")
        if (unicode != null) {
            pass("Unicode string parsed: %(unicode["emoji"])")
        } else {
            fail("Unicode string failed")
        }

        // Empty string
        var emptyStr = Json.parse("\"\"")
        if (emptyStr == "") {
            pass("Empty string parsed correctly")
        } else {
            fail("Empty string failed")
        }

        // Deeply nested
        var deep = Json.parse("{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":5}}}}}")
        if (deep != null && deep["a"]["b"]["c"]["d"]["e"] == 5) {
            pass("Deep nesting: e=%(deep["a"]["b"]["c"]["d"]["e"])")
        } else {
            fail("Deep nesting failed")
        }
    }

    static pass(msg) {
        System.print("  [PASS] %(msg)")
        __results.add({"pass": true, "msg": msg})
    }

    static fail(msg) {
        System.print("  [FAIL] %(msg)")
        __results.add({"pass": false, "msg": msg})
        __allPassed = false
    }

    static update(dt) {
        // No-op
    }

    static render() {
        var y = -300

        Render.text(__font, "JSON API Test (C++ Implementation)", 0, y, 1, 0xffffffff, 0x00000000, Render.spriteCenterX)
        y = y + 30
        Render.text(__font, "Check console output for test results", 0, y, 1, 0xffaaaaaa, 0x00000000, Render.spriteCenterX)
        y = y + 50

        // Show pass/fail count
        var passCount = 0
        var failCount = 0
        for (r in __results) {
            if (r["pass"]) {
                passCount = passCount + 1
            } else {
                failCount = failCount + 1
            }
        }

        var statusColor = __allPassed ? 0xff88ff88 : 0xffff8888
        var statusText = __allPassed ? "ALL TESTS PASSED" : "SOME TESTS FAILED"

        Render.text(__font, statusText, 0, y, 1, statusColor, 0x00000000, Render.spriteCenterX)
        y = y + 30
        Render.text(__font, "Passed: %(passCount) / Failed: %(failCount)", 0, y, 1, 0xffcccccc, 0x00000000, Render.spriteCenterX)
    }
}
