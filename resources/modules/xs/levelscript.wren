/// A compiled, reusable LevelScript program: a stateless generator factory.
/// Mirrors the C++ embedding API's `ls::generator` (see ls.hpp) - a program
/// is a pure function of (seed, params).
foreign class LsGenerator {

    /// Compiles LevelScript source text, labeling diagnostics "generator".
    construct compile(source: String) {
        compile_(source, "generator")
    }

    /// Compiles LevelScript source text. `name` labels diagnostics
    /// ("dungeon.ls:12:3: error: ...").
    construct compile(source: String, name: String) {
        compile_(source, name)
    }

    /// Private: does the actual compile, in place, after allocate().
    foreign compile_(source: String, name: String)

    /// True if compilation succeeded.
    foreign isValid -> Bool

    /// Formatted diagnostics when compilation failed; "" when it succeeded.
    foreign error -> String

    /// Formatted warnings of a successful compile; "" when there are none.
    foreign warnings -> String

    /// Number of program statements (progress denominators).
    foreign statementCount -> Num

    /// Value mask, qualified by tagset: tag("geo.wall") -> that value's bit;
    /// also resolves a named union to its composite mask. Masks are per
    /// tagset, valid for every layer of that tagset. 0 if unknown (an
    /// unknown tag ORed into a composite mask degrades correctly).
    foreign tag(qualified: String) -> Num

    /// Private: runs the whole program (seed, param names, param values) -> level.
    foreign generate_(seed: Num, paramNames: List, paramValues: List) -> LsLevel

    /// Runs the whole program deterministically for a given seed.
    generate(seed) {
        return generate_(seed, [], [])
    }

    /// Runs the whole program with parameter overrides (unknown names are ignored).
    generate(seed, params: Map) {
        var names = []
        var values = []
        for (key in params.keys) {
            names.add(key)
            values.add(params[key])
        }
        return generate_(seed, names, values)
    }

    /// Private: starts a progressive run.
    foreign run_(seed: Num, stepMode: Num, paramNames: List, paramValues: List) -> LsRun

    /// Starts a progressive run, advancing one statement per step().
    run(seed) {
        return run_(seed, LsStepMode.statement, [], [])
    }

    /// Starts a progressive run with an explicit step granularity
    /// (LsStepMode.statement or LsStepMode.application).
    run(seed, stepMode) {
        return run_(seed, stepMode, [], [])
    }

    /// Starts a progressive run with an explicit step granularity and
    /// parameter overrides.
    run(seed, stepMode, params: Map) {
        var names = []
        var values = []
        for (key in params.keys) {
            names.add(key)
            values.add(params[key])
        }
        return run_(seed, stepMode, names, values)
    }
}

/// step() granularity for LsGenerator.run().
class LsStepMode {
    /// step() advances one whole rule-application statement at a time.
    static statement -> Num { 0 }

    /// step() advances one single pattern application at a time (finer
    /// granularity, more steps per statement).
    static application -> Num { 1 }
}

/// One generated outcome: the stack of co-registered grids. A self-contained
/// value - it owns its cells and outlives the generator that produced it.
/// Not to be created directly - use LsGenerator.generate/run.
foreign class LsLevel {

    /// Width, in cells, of the level.
    foreign width -> Num

    /// Height, in cells, of the level.
    foreign height -> Num

    /// Number of grid layers in the level.
    foreign count -> Num

    /// Name of a layer, by declaration order (0-based).
    foreign layerName(index: Num) -> String

    /// The grid for a named layer.
    foreign [name: String] -> LsGrid

    /// The grid for a layer, by declaration order (0-based).
    foreign layer(index: Num) -> LsGrid
}

/// One layer of a generated level. Shares ownership of its data, so a grid
/// handed to Wren can outlive the level object it came from.
/// Not to be created directly - use LsLevel[name] or LsLevel.layer(index).
foreign class LsGrid {

    /// Name of the layer this grid was pulled from ("" if invalid).
    foreign name -> String

    /// True if this is a number layer (vs. a tag layer).
    foreign isNumber -> Bool

    /// Cell at (x, y): for a tag layer, the stored value mask (a single bit
    /// for a normal cell, multiple bits for a union write); for a number
    /// layer, the stored number. `null` for an empty cell, out of range, or
    /// an invalid grid - for number layers, `null` is the only reliable
    /// emptiness test (a cell can legitimately store the number 0 or a
    /// negative number).
    foreign [x: Num, y: Num] -> Num?

    /// True if the cell's stored mask overlaps `mask` at all - the one
    /// query that stays correct for a union cell (multiple value bits set).
    foreign has(x: Num, y: Num, mask: Num) -> Bool

    /// Name of a single-bit value mask ("" if `mask` isn't exactly one
    /// value bit, or is out of range / a number layer).
    foreign valueName(mask: Num) -> String
}

/// One in-flight progressive generation run. Not to be created directly -
/// use LsGenerator.run.
foreign class LsRun {

    /// Advances by the granularity fixed at LsGenerator.run(); false when done.
    foreign step() -> Bool

    /// A copy of the current state of the run: committed statements plus
    /// this batch's applications so far. Cheap to call every few steps to
    /// animate generation (e.g. driven by Fiber.yield()).
    foreign snapshot -> LsLevel

    /// Drains whatever remains of the run and returns the finished level.
    foreign finish() -> LsLevel
}
