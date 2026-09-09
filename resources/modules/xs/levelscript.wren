/// Handle to a compiled LevelScript program: a reusable, stateless
/// generator factory. Not to be created directly - use LevelScript.compile.
foreign class LevelScriptProgram {}

/// Handle to one generated level outcome: a stack of correlated grids.
/// Not to be created directly.
foreign class LevelScriptLevel {}

/// Handle to one in-flight progressive generation run.
/// Not to be created directly - use LevelScript.begin.
foreign class LevelScriptRun {}

/// Procedural level generation via the LevelScript DSL.
/// A program transforms a stack of correlated grids through pattern-rewrite
/// rules; a generated level is a pure function of (seed, params).
class LevelScript {

    /// Compiles LevelScript source text into a reusable program.
    /// `name` labels diagnostics ("dungeon.ls:12:3: error: ...").
    foreign static compile_(source: String, name: String) -> LevelScriptProgram

    /// Compiles LevelScript source text, labeling diagnostics "generator".
    static compile(source: String) -> LevelScriptProgram {
        return compile_(source, "generator")
    }

    /// Compiles LevelScript source text with a diagnostics label.
    static compile(source: String, name: String) -> LevelScriptProgram {
        return compile_(source, name)
    }

    /// True if compilation succeeded.
    foreign static isValid(program: LevelScriptProgram) -> Bool

    /// Formatted diagnostics when compilation failed; "" when it succeeded.
    foreign static error(program: LevelScriptProgram) -> String

    /// Formatted warnings of a successful compile; "" when there are none.
    foreign static warnings(program: LevelScriptProgram) -> String

    /// Tag value id, qualified by tagset (e.g. "geo.wall"). -1 if unknown.
    /// Valid for every layer sharing that tagset.
    foreign static tag(program: LevelScriptProgram, qualifiedName: String) -> Num

    /// Private: runs the whole program (seed, param names, param values) -> level.
    foreign static generate_(program: LevelScriptProgram, seed: Num, paramNames: List, paramValues: List) -> LevelScriptLevel

    /// Runs the whole program deterministically for a given seed.
    static generate(program: LevelScriptProgram, seed: Num) -> LevelScriptLevel {
        return generate_(program, seed, [], [])
    }

    /// Runs the whole program with parameter overrides (unknown names are ignored).
    static generate(program: LevelScriptProgram, seed: Num, params: Map) -> LevelScriptLevel {
        var names = []
        var values = []
        for (key in params.keys) {
            names.add(key)
            values.add(params[key])
        }
        return generate_(program, seed, names, values)
    }

    /// Width, in cells, of a generated level.
    foreign static width(level: LevelScriptLevel) -> Num

    /// Height, in cells, of a generated level.
    foreign static height(level: LevelScriptLevel) -> Num

    /// Number of grid layers in a generated level.
    foreign static layerCount(level: LevelScriptLevel) -> Num

    /// Name of a layer, by declaration order (0-based).
    foreign static layerName(level: LevelScriptLevel, index: Num) -> String

    /// Cell value at (x, y) on a named layer: the tag value id, or the stored
    /// number for number layers; -1 when empty, out of range, or invalid.
    foreign static at(level: LevelScriptLevel, layerName: String, x: Num, y: Num) -> Num

    /// Name of a tag value id on a named layer ("" for number layers or an unknown id).
    foreign static valueName(level: LevelScriptLevel, layerName: String, valueId: Num) -> String

    /// Private: starts a progressive run.
    foreign static begin_(program: LevelScriptProgram, seed: Num, stepMode: Num, paramNames: List, paramValues: List) -> LevelScriptRun

    /// Starts a progressive run, advancing one statement per step().
    static begin(program: LevelScriptProgram, seed: Num) -> LevelScriptRun {
        return begin_(program, seed, statementStep, [], [])
    }

    /// Starts a progressive run with an explicit step granularity (statementStep
    /// or applicationStep).
    static begin(program: LevelScriptProgram, seed: Num, stepMode: Num) -> LevelScriptRun {
        return begin_(program, seed, stepMode, [], [])
    }

    /// Starts a progressive run with an explicit step granularity (statementStep
    /// or applicationStep) and parameter overrides.
    static begin(program: LevelScriptProgram, seed: Num, stepMode: Num, params: Map) -> LevelScriptRun {
        var names = []
        var values = []
        for (key in params.keys) {
            names.add(key)
            values.add(params[key])
        }
        return begin_(program, seed, stepMode, names, values)
    }

    /// Advances a run by one unit (a statement, or an application - see
    /// stepMode); returns false once generation is complete.
    foreign static step(run: LevelScriptRun) -> Bool

    /// A copy of the current state of a run: committed statements plus this
    /// batch's applications so far. Cheap to call every few steps to animate
    /// generation (e.g. driven by Fiber.yield()).
    foreign static snapshot(run: LevelScriptRun) -> LevelScriptLevel

    /// Drains whatever remains of a run and returns the finished level.
    foreign static finish(run: LevelScriptRun) -> LevelScriptLevel

    /// step() advances one whole rule-application statement at a time.
    static statementStep -> Num { 0 }

    /// step() advances one single pattern application at a time (finer
    /// granularity, more steps per statement).
    static applicationStep -> Num { 1 }
}
