import "xs/core" for Data, Input, Render, File
import "xs/levelscript" for LsGenerator, LsStepMode
import "background" for Background

// Demonstrates embedding LevelScript (.ls) programs in xs: compiles a few
// bundled generators, runs one progressively (cell-by-cell, animated via
// Fiber.yield - same pattern samples/rogue's generators.wren uses for its
// own hand-written Wren generators), and renders the result as tinted
// tiles from samples/grid's sprite sheet (same Render.createGridSprite
// pattern as samples/grid/game.wren).
class Game {

    static generating { 0 }
    static done        { 1 }
    static tileSize     { 16 }

    static initialize() {
        __background = Background.new()

        __palette = [
            0x2b2b2bff, 0x5c5c5cff, 0x8a8a8aff, 0xc0c0c0ff,
            0x4d89f2ff, 0x2feff9ff, 0xed3bf9ff, 0x72ffa1ff
        ]

        // Generic tile bank, cycled by declaration-order value index (see
        // bitIndex() below) - these are the same known-good sprite sheet
        // indices samples/grid uses (columns = 49, rows = 22), not tied to
        // any one program's tag semantics.
        var image = Render.loadImage("[game]/assets/monochrome-transparent_packed.png")
        var tileIndices = [624, 51, 52, 53, 5, 1, 6, 3]
        __tiles = []
        for (index in tileIndices) {
            __tiles.add(Render.createGridSprite(image, 49, 22, index))
        }

        __names = ["dungeon", "cave", "walk"]
        __programs = []
        for (name in __names) {
            var source = File.read("[game]/levels/%(name).ls")
            var program = LsGenerator.compile(source, name)
            if (!program.isValid) {
                System.print("LevelScript: failed to compile %(name).ls: %(program.error)")
            } else {
                var warnings = program.warnings
                if (warnings != "") System.print("LevelScript: %(name).ls warnings: %(warnings)")
            }
            __programs.add(program)
        }

        __programIndex = -1
        __seed = -1
        __level = null
        __time = 0.0
        __state = Game.done

        startGeneration()
    }

    // (Re)starts generation for whatever the Program/Seed data values
    // currently select, driven progressively through a Fiber - each step
    // yields a brake duration, exactly like samples/rogue's __genFiber.
    static startGeneration() {
        __programIndex = Data.getNumber("Program|Dungeon|Cave|Walk").truncate
        __seed = Data.getNumber("Seed")
        __level = null
        __time = 0.0
        __state = Game.generating

        System.print("startGeneration: program=%(__names[__programIndex]) seed=%(__seed)")

        var program = __programs[__programIndex]
        if (!program.isValid) {
            __state = Game.done
            return
        }

        var brake = Data.getNumber("Step Brake")
        var seed = __seed
        // LsStepMode.application (not the default statement) so a single
        // "some(max=200) rule" statement animates one rule application at a
        // time, instead of jumping straight from its start state to its end
        // state in a single step().
        __genFiber = Fiber.new {
            var run = program.run(seed, LsStepMode.application)
            while (run.step()) {
                __level = run.snapshot
                Fiber.yield(brake)
            }
            __level = run.finish()
            return 0.0
        }
    }

    // Dumps a layer to the console as ASCII, one character per cell (first
    // letter of the tag's name, or the raw number for a number layer, "."
    // for empty) - same convention levelscript's own *.expected fixtures use,
    // so the output is directly comparable to those.
    static dump(level, layerIndex) {
        var width = level.width
        var height = level.height
        var layerName = level.layerName(layerIndex)
        var grid = level.layer(layerIndex)
        System.print("=== %(layerName) (%(width)x%(height)) ===")
        for (y in 0...height) {
            var row = ""
            for (x in 0...width) {
                var value = grid[x, y]
                if (value < 0) {
                    row = row + ". "
                } else {
                    var name = grid.valueName(value)
                    row = row + (name == "" ? "%(value) " : name[0] + " ")
                }
            }
            System.print(row)
        }
    }

    static update(dt) {
        __background.update(dt)

        var programIndex = Data.getNumber("Program|Dungeon|Cave|Walk").truncate
        var seed = Data.getNumber("Seed")
        var regenerate = Input.getKeyOnce(Input.keySpace)
        if (programIndex != __programIndex || seed != __seed || regenerate) {
            startGeneration()
        }

        if (__state == Game.generating) {
            if (Data.getBool("Visualize Generation")) {
                __time = __time - dt
                if (__time <= 0.0) {
                    if (!__genFiber.isDone) {
                        __time = __genFiber.call()
                    } else {
                        __state = Game.done
                        dump(__level, 0)
                    }
                }
            } else {
                while (!__genFiber.isDone) __genFiber.call()
                __state = Game.done
                dump(__level, 0)
            }
        }
    }

    static render() {
        __background.render()

        if (__level == null) return

        var width = __level.width
        var height = __level.height
        var grid = __level.layer(0)

        var s = Game.tileSize
        var sx = (width - 1) * -s / 2
        var sy = (height - 1) * -s / 2

        for (y in 0...height) {
            for (x in 0...width) {
                var value = grid[x, y]
                if (value < 0) continue

                // grid[x, y] is a value MASK for a tag layer (bit 1..30,
                // spec §3), not a small sequential id - convert back to a
                // 0-based index to cycle through the fixed tile/color banks.
                var index = bitIndex(value)
                var tile = __tiles[index % __tiles.count]
                var color = __palette[index % __palette.count]
                Render.sprite(
                    tile,
                    sx + x * s, sy + y * s,
                    0.0, 1.0, 0.0,
                    color, 0x0,
                    Render.spriteCenter)
            }
        }
    }

    // Lowest set value bit of a mask (bit 1..30) as a 0-based index:
    // wall (1<<1) -> 0, floor (1<<2) -> 1, and so on.
    static bitIndex(mask) {
        var b = 1
        while ((mask & (1 << b)) == 0) b = b + 1
        return b - 1
    }
}
