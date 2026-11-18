import "xs/core" for Data, Input, Render
import "xs/levelscript" for LsGenerator, LsStepMode
import "xs/tools" for ImageBuilder

class Game {
    static generating { 0 }
    static done       { 1 }
    static tileSize    { 8 }
    static tileColumns { 16 }
    static tileRows    { 11 }
    static paletteSize { 16 }

    static hsvColor(h) {
        var s = 0.75
        var v = 0.85
        var i = (h * 6).truncate
        var f = h * 6 - i
        var p = v * (1 - s)
        var q = v * (1 - f * s)
        var t = v * (1 - (1 - f) * s)
        var r = 0
        var g = 0
        var b = 0
        var sec = i % 6
        if (sec == 0) {
            r = v
            g = t
            b = p
        } else if (sec == 1) {
            r = q
            g = v
            b = p
        } else if (sec == 2) {
            r = p
            g = v
            b = t
        } else if (sec == 3) {
            r = p
            g = q
            b = v
        } else if (sec == 4) {
            r = t
            g = p
            b = v
        } else {
            r = v
            g = p
            b = q
        }
        return ImageBuilder.pack((r*255).truncate, (g*255).truncate, (b*255).truncate, 255)
    }

    static buildPalette() {
        var n = Game.paletteSize
        var s = Game.tileSize
        var builder = ImageBuilder.new(n * s, s)
        for (i in 0...n) {
            var color = Game.hsvColor(i / n)
            for (py in 0...s) {
                for (px in 0...s) {
                    builder.setPixel(i * s + px, py, color)
                }
            }
        }
        return builder.build()
    }

    static initialize() {
        __image = Render.loadImage("[game]/assets/kenney_micro_roguelike.png")
        __spriteCache = {}

        // One palette texture, one sprite per hue slot, cached by value name
        __paletteImage = Game.buildPalette()
        __paletteSprites = {}   // value name → sprite id
        __nextSlot = 0

        __gen = LsGenerator.compileFile("[game]/levels/roguelike_decorated.lvs")
        if (!__gen.isValid) {
            System.print("compile error: %(__gen.error)")
        } else {
            var w = __gen.warnings
            if (w != "") System.print("warnings: %(w)")
        }

        __seed = -1
        __level = null
        __time = 0.0
        __state = Game.done
        startGeneration()
    }

    static paletteSprite(name) {
        if (!__paletteSprites.containsKey(name)) {
            var slot = __nextSlot % Game.paletteSize
            __nextSlot = __nextSlot + 1
            __paletteSprites[name] = Render.createGridSprite(__paletteImage, Game.paletteSize, 1, slot)
        }
        return __paletteSprites[name]
    }

    static startGeneration() {
        if (!__gen.isValid) return
        __seed = Data.getNumber("Seed")
        __level = null
        __time = 0.0
        __state = Game.generating
        var seed = __seed
        var brake = Data.getNumber("Step Brake")
        __fiber = Fiber.new {
            var run = __gen.run(seed, LsStepMode.application, "main_decorated")
            while (run.step()) {
                __level = run.snapshot
                Fiber.yield(brake)
            }
            __level = run.finish()
            return 0.0
        }
    }

    static update(dt) {
        var seed = Data.getNumber("Seed")
        if (seed != __seed || Input.getKeyOnce(Input.keySpace)) {
            startGeneration()
        }

        if (__state == Game.generating) {
            if (Data.getBool("Step")) {
                __time = __time - dt
                if (__time <= 0.0) {
                    if (!__fiber.isDone) {
                        __time = __fiber.call()
                    } else {
                        __state = Game.done
                    }
                }
            } else {
                while (!__fiber.isDone) __fiber.call()
                __state = Game.done
            }
        }
    }

    static drawGraph(level, ox, oy, s) {
        var graph = level.graph("map")
        var n = graph.nodeCount
        if (n == 0) return

        var w = level.width
        var h = level.height

        // Find each node's centroid by scanning all cells once
        var sumX = List.filled(n, 0)
        var sumY = List.filled(n, 0)
        var cnt  = List.filled(n, 0)
        for (y in 0...h) {
            for (x in 0...w) {
                var node = graph.nodeAt(x, y)
                if (node == null) continue
                var ni = node.truncate
                sumX[ni] = sumX[ni] + x
                sumY[ni] = sumY[ni] + y
                cnt[ni]  = cnt[ni]  + 1
            }
        }

        // Edges first, then node dots on top
        Render.dbgColor(0xFFFFFF55)
        for (ni in 0...n) {
            if (cnt[ni] == 0) continue
            var cx = ox + (sumX[ni] / cnt[ni]) * s
            var cy = oy + (sumY[ni] / cnt[ni]) * s
            for (nj in graph.neighbors(ni)) {
                var j = nj.truncate
                if (j > ni && cnt[j] > 0) {
                    Render.dbgLine(cx, cy, ox + (sumX[j] / cnt[j]) * s, oy + (sumY[j] / cnt[j]) * s)
                }
            }
        }
        Render.dbgColor(0xFF8080FF)
        for (ni in 0...n) {
            if (cnt[ni] == 0) continue
            Render.dbgSquare(ox + (sumX[ni] / cnt[ni]) * s, oy + (sumY[ni] / cnt[ni]) * s, 3)
        }
    }

    static render() {
        if (__level == null) return
        var w = __level.width
        var h = __level.height
        var s = Game.tileSize
        var ox = (w - 1) * -s / 2
        var oy = (h - 1) * -s / 2

        if (__state == Game.generating) {
            // Tag layers: colored palette blocks
            for (i in 0...__level.count) {
                var grid = __level.layer(i)
                if (grid.isNumber) continue
                for (y in 0...h) {
                    for (x in 0...w) {
                        var mask = grid[x, y]
                        if (mask == null) continue
                        var name = grid.valueName(mask)
                        if (name == "") continue
                        Render.sprite(Game.paletteSprite(name),
                            ox + x * s, oy + y * s, 0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
                    }
                }
            }
            // Tiles layer: real sprites on top wherever already filled
            var tiles = __level["tiles"]
            for (y in 0...h) {
                for (x in 0...w) {
                    var idx = tiles[x, y]
                    if (idx == null) continue
                    var key = idx.truncate
                    if (!__spriteCache.containsKey(key)) {
                        __spriteCache[key] = Render.createGridSprite(__image, Game.tileColumns, Game.tileRows, key)
                    }
                    Render.sprite(__spriteCache[key], ox + x * s, oy + y * s,
                        0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
                }
            }
        } else {
            // Final render: tile sprites from the kenney tileset
            var tiles = __level["tiles"]
            for (y in 0...h) {
                for (x in 0...w) {
                    var idx = tiles[x, y]
                    if (idx == null) continue
                    var key = idx.truncate
                    if (!__spriteCache.containsKey(key)) {
                        __spriteCache[key] = Render.createGridSprite(__image, Game.tileColumns, Game.tileRows, key)
                    }
                    Render.sprite(__spriteCache[key], ox + x * s, oy + y * s,
                        0.0, 1.0, 0.0, 0xFFFFFFFF, 0x0, Render.spriteCenter)
                }
            }
        }

        Game.drawGraph(__level, ox, oy, s)
    }
}
