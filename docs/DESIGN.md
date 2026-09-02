# xs Engine Design Document

## 1. Purpose and Scope

This document describes the architecture of the **xs** engine: its subsystems, how
they fit together, and — just as importantly — *why* they were built the way they
were. It is intended for contributors and students who need to understand the
engine beyond the public API in order to extend, port, or teach with it.

A design document like this typically covers:

- **Goals and non-goals** — what the project is trying to achieve, and what it
  deliberately avoids doing.
- **High-level architecture** — the major subsystems and how control/data flow
  between them.
- **Key abstractions** — the core types and interfaces that the rest of the
  system is built around.
- **Cross-cutting concerns** — things like platform portability, data/asset
  pipelines, and performance that touch every subsystem.
- **Design rationale** — the trade-offs considered and why a particular option
  was chosen, especially where the choice is not obvious from the code alone.
- **Alternatives considered / rejected** — so future contributors don't
  re-litigate settled decisions without knowing why they were made.
- **Open questions / future work** — known limitations or areas flagged for
  future iteration.

This document follows that structure, adapted to xs's current codebase
(`/code`, `/platforms`, `/resources/modules`).

## 2. Goals and Non-Goals

**Goals**

- Be the **smallest practical** 2D game engine usable for teaching introductory
  game engine programming (xs = "extra small").
- Provide a **scriptable, hot-reloadable** game layer via [Wren](https://wren.io/),
  so students can iterate on gameplay without recompiling or restarting the
  engine.
- Support **multiple platforms** (PC/Windows, Linux, macOS/iOS/tvOS, PS5,
  Switch) from a single C++ core, with platform code isolated behind thin
  interfaces.
- Make it easy to **package** a game (assets + scripts) into a single
  distributable file.

**Non-goals**

- xs is not trying to be a general-purpose, production AAA engine (no 3D
  renderer, no complex material/shader graph, no built-in networking).
- It does not attempt to abstract away every rendering API difference with a
  full RHI; it currently ships one native renderer per platform family
  (OpenGL for PC/Linux, Metal for Apple, native platform APIs for consoles).
- Wren is the only supported scripting language — there is no plan for a
  plugin system supporting arbitrary languages.

## 3. High-Level Architecture

```
        xs::main() / xs::dispatch()
                    |
                    v
        +---------------------+
        |   xs::initialize()  |   (code/xs.cpp)
        +---------------------+
                    |
    log -> account -> fileio -> data -> script::configure
                    |
    device -> render -> input -> audio -> simple_audio -> inspector -> script::initialize
                    |
                    v
        +---------------------+
        |   xs::update(dt)    |  <-- main loop, called every frame
        +---------------------+
         |     |      |      |
       device input  script  render
       poll   update update  clear/render
                render  (game logic + draw calls)
                    |
                    v
        +---------------------+
        |   xs::shutdown()    |  (reverse-order teardown)
        +---------------------+
```

`xs.hpp`/`xs.cpp` is the composition root: it owns the ordered
initialize/update/shutdown sequence and nothing else. Every other subsystem
(`device`, `render`, `script`, `audio`, `input`, `data`, `fileio`, `inspector`,
`account`, `packager`) is a free-function namespace with its own
`initialize()`/`shutdown()` pair, no shared global object, and no dependency
on being called from a particular translation unit. This keeps the "what
starts before what" logic in exactly one, easily auditable place.

### 3.1 Subsystems

| Subsystem | Header | Responsibility |
|---|---|---|
| `xs` | `xs.hpp` | Composition root: CLI dispatch, init/update/shutdown ordering, run mode |
| `device` | `device.hpp` | Window/platform lifecycle, frame begin/end, event polling |
| `render` | `render.hpp` | Sprites, shapes, text, images, fonts, debug primitives, draw stats |
| `script` | `script.hpp` | Wren VM lifecycle, module loading, foreign method/class binding |
| `input` | `input.hpp` | Keyboard/mouse/gamepad/touch abstraction |
| `audio` / `simple_audio` | `audio.hpp` / `simple_audio.hpp` | Music/streaming audio vs. simple one-shot SFX |
| `data` | `data.hpp` | Typed key/value persisted state (project/game/save/user scoped) |
| `fileio` | `fileio.hpp` | Wildcard-based path resolution, reading/writing, packaged vs. loose files |
| `packager` | `packager.hpp` | Serializes a project folder into a single `.xs` package |
| `configuration` | `configuration.hpp` | Reads project settings (resolution, scaling, title) |
| `account` | `account.hpp` | Platform account/identity hook (e.g., console sign-in) |
| `inspector` | `inspector.hpp` | ImGui-based in-engine debug UI / entity inspector |
| `profiler` | `profiler.hpp` | Lightweight frame profiling |
| `log` | `log.hpp` | Structured logging categories (engine, script, etc.) |
| `version` | `version.cpp` | Engine version encode/decode, used by packages and CLI |

### 3.2 Platform Abstraction

`code/platform.hpp` selects, at compile time via preprocessor defines
(`PLATFORM_PS5`, `PLATFORM_OPENGL`, `PLATFORM_APPLE`, ...), which concrete
header is included. Each platform directory under `/platforms` (`pc`, `linux`,
`apple`, `prospero`, `nx`, `null`) provides the platform-specific
implementation file for a subsystem (e.g. `render_apple.mm`,
`device_apple.mm`, `fileio_apple.mm`), while the public interface
(`device.hpp`, `render.hpp`, `fileio.hpp`, ...) stays identical across
platforms. The `null` platform exists purely to let the engine (and CI)
compile/link without a real backend, e.g. for the `packaging` run mode which
needs no window or renderer at all.

### 3.3 Scripting Layer

The C++ core exposes a small, stable **foreign function interface** to Wren
(`script::bind`), and the bulk of the engine's *game-facing* API (entity
components, math helpers, containers, tooling) is implemented in Wren itself,
under `resources/modules/xs/*.wren` (`core.wren`, `ec.wren`, `components.wren`,
`math.wren`, `containers.wren`, `tools.wren`). C++ implements the
performance/platform-sensitive primitives (rendering, audio, input, IO);
Wren implements the higher-level, frequently-iterated-on game framework
(entity/component system, math/container utilities) on top of those
primitives.

### 3.4 Data & Packaging

`fileio` resolves paths through **wildcards** (`[game]`, `[shared]`, `[user]`,
...) rather than hardcoded absolute/relative paths, so the same script and
asset paths work whether the engine is running from a loose project folder
(`run_mode::development`) or from a compiled `.xs` package
(`run_mode::packaged`). `packager` serializes a project's files (via
`cereal`) into a single versioned, optionally-compressed archive, keyed by
the same wildcard-prefixed paths, so packaging is just "redirect the wildcard
resolution to read from an archive" rather than a parallel code path.

## 4. Key Design Choices and Their Motivation

### 4.1 Wren for gameplay scripting, with hot reload

**Choice:** Game logic is written in Wren and reloaded at runtime rather than
compiled as part of the C++ binary.

**Motivation:** The engine's primary use case is teaching — the faster a
student can change a line of gameplay code and see the result, the better the
feedback loop for learning. Wren is small, fast to embed, and fast to
reload (per the project's own description, "hot-reloadable in
microseconds"), which avoids the multi-second-to-minutes edit/compile/link/run
cycle a pure C++ game loop would require. Keeping the *engine* in C++ and only
the *game* in Wren means students get native performance for the systems that
need it (rendering, audio) while working exclusively in a simpler, garbage
collected, dynamically typed language for gameplay.

### 4.2 Split between "hard" C++ core and "soft" Wren standard library

**Choice:** Fundamental game-framework concepts like the entity/component
system (`ec.wren`, `components.wren`) and math helpers (`math.wren`) are
implemented in Wren, layered on top of a small set of C++ foreign functions,
instead of being implemented (and hot-path-optimized) in C++.

**Motivation:** These are the parts of the API students read, modify, and
debug most often. Implementing them in Wren means: (1) they can be inspected
and patched without recompiling the engine, (2) engine maintainers can
iterate on the framework API surface without touching C++/build
infrastructure, and (3) it keeps the C++ core small, matching the "extra
small" goal — the C++ surface is limited to primitives that actually need to
be native (draw calls, audio mixing, file IO, input polling).

### 4.3 Free-function namespaces instead of singleton classes/objects

**Choice:** Every subsystem (`render`, `script`, `audio`, `device`, ...) is a
namespace of free functions with internal (often file-local) state, rather
than a class instance passed around or a singleton object with `instance()`
accessors.

**Motivation:** There is exactly one instance of each subsystem per process
(xs does not support multiple simultaneous engine instances), so a class
wrapper would only add indirection without adding flexibility. Free
functions keep call sites simple (`render::sprite(...)` instead of
`Engine::instance().renderer().sprite(...)`), make the Wren FFI bindings
straightforward (binding a free function pointer is simpler than binding a
member function through an instance), and make the initialize/shutdown order
in `xs.cpp` trivial to read and audit top-to-bottom.

### 4.4 Wildcard-based virtual file system

**Choice:** All file access in the engine goes through `fileio`'s wildcard
resolution (`[game]`, `[shared]`, `[user]`, ...) instead of raw paths.

**Motivation:** A game must run identically whether it's a loose folder on a
developer's machine, a packaged `.xs` file, or a project running from a
platform-specific sandboxed storage location (each platform has different
rules for where user/save data lives). Centralizing path resolution behind
wildcards means every other subsystem (scripts, data, packager) can be
written once against symbolic paths, and only `fileio` needs to know the
concrete difference between `development`, `packaged`, and `packaging` run
modes, or between platform storage conventions.

### 4.5 Single-file `.xs` packages via `cereal`

**Choice:** Shipping a game means serializing the whole project tree into one
binary file with a magic number, version header, and per-entry
compression flag, rather than shipping a loose folder of assets.

**Motivation:** A single file is simpler to distribute, faster to open (one
file handle instead of many small file-system calls, which matters
especially on consoles with slower/virtualized file systems), and lets the
engine embed a version number to detect format drift between the packaging
tool and the runtime. Using `cereal` for serialization avoids hand-rolling a
binary format and its versioning/endianness pitfalls. Per-entry compression
(rather than whole-archive compression) allows lazy/partial loading of
individual assets later without decompressing the entire package.

### 4.6 Compile-time platform selection over a runtime backend abstraction

**Choice:** Platform backends (`render_apple.mm`, `device_apple.mm`, PC/OpenGL
equivalents, etc.) are selected via preprocessor defines and separate
translation units per platform, rather than a runtime-polymorphic backend
interface (e.g. virtual `IRenderer`).

**Motivation:** xs targets platforms (PS5, Switch) where a given binary only
ever runs on one hardware target, so runtime backend switching provides no
value and would add virtual-dispatch overhead and an extra abstraction layer
for no real benefit. Compile-time selection also lets platform-specific code
use platform-native language extensions directly (Objective-C++ for Apple,
console vendor SDKs) without needing to shoehorn them through a
lowest-common-denominator C++ interface.

### 4.7 Explicit `run_mode` (development / packaged / packaging) instead of implicit detection everywhere

**Choice:** `xs::run_mode` is a single, explicitly-set enum threaded through
the engine, rather than having each subsystem independently infer "are we
running from a package?" from its own heuristics.

**Motivation:** Packaging (`run_mode::packaging`) intentionally initializes
only the minimal subsystems needed to read a project and write a package
(`log`, `fileio`, `data`, `script::configure`) — explicitly skipping
`device`/`render`/`audio`/`input` — so the CLI packaging tool can run
headless on a build machine with no window/GPU/audio device available. A
single, explicit mode flag makes this "headless subset of initialization"
easy to reason about and keeps the decision in one place (`xs::package` vs.
`xs::initialize`) instead of scattered availability checks.

### 4.8 CLI-first entry point with subcommands

**Choice:** `xs::dispatch` implements `run` / `package` / `version`
subcommands (via `argparse`) on desktop platforms, with `run` auto-detecting
whether its argument is a `.xs` package or a project folder.

**Motivation:** The same binary is used both by students running a project
during development and by the packaging pipeline producing a shippable
build, so a single CLI with explicit subcommands avoids needing separate
tools/binaries for "run" vs. "package" vs. "print version" while keeping each
use case's argument surface small and self-documenting.

## 5. Alternatives Considered

- **Embedding a heavier scripting language (Lua, Python, JS/QuickJS):**
  rejected in favor of Wren for its small footprint, fast reload times, and
  built-in class/fiber model that maps naturally onto an entity/component
  update loop.
- **A full render hardware interface (RHI) abstracting Metal/OpenGL/console
  APIs behind one interface:** rejected as unnecessary complexity given xs
  only needs 2D sprite/shape/text rendering and each platform already has a
  single, fixed native backend.
- **Loose-folder-only distribution (no packager):** rejected because console
  platforms and simpler distribution needs favor a single-file format with
  version metadata.

## 6. Open Questions / Future Work

- Package entries currently load file `data` fully into memory on load, with a
  `// may be removed later for lazy loading` note in `packager.hpp` —
  streaming/lazy asset loading from packages is a known future improvement.
- There is currently one native renderer per platform family; if a future
  platform doesn't fit `PLATFORM_PS5` / `PLATFORM_OPENGL` / `PLATFORM_APPLE`,
  `platform.hpp` will need a new branch and a fully new backend.
