<img src="img/top_banner.png" alt="xs" width="100%">

# xs - Design Document

Version 26.x · Bojan Endrovski


## What this is

xs is an extra small game engine, aimed at quick iterations on small games. A

The core is C++, games are Wren.

It's built first for how its author makes games: code, and a lot of procedural generation.
It also teaches an engine programming course. The first sets the constraints, the second is
a use it happens to fit.

This document records the decisions, what they were chosen over, and what each one costs.
It's what proposals get argued against. It isn't an API reference and isn't a tutorial.


## Philosophy 

Everything follows from these key points.

**Code-first.** Code is a creative expression and in xs code is the main way to create games. Game logic is written in a scripting language, through a minimal three-method *Game* class. The engine's UI is minimal and mostly used for debugging and inspection. 

**Rapid ideation.** Scripts should be hot-reload without restarting and a new project should take minutes to get off the ground. The UI should exposes game parameters at runtime for live editing.

**No features, no bugs.** Every line of API is promise that needs to be kept for the lifetime of the project. xs actively tries to maintain minimal feature set, but one the works reliably at all times.

**Tiny, but shippable** More than experiments, a game should be shippable on most platforms with xs.

**Minimal cognitive load.** xs aims to reduce the number of choices that a game developer would need to make at any given time.


Flat
namespaces, no inheritance, no hidden control flow, primitives at the boundary. It reaches
the surface too: the codebase is lowercase throughout, commit messages included, so nothing
shouts.


## Decisions

All decision stem from the key points above.

### Programming Languages

From there, one can choose which modules to use for their game.

**C++ as implementation language.**
All game-related APIs are C/C++ primarily. C++ remains the fastest language, powering most systems on the planet. This does not imply we have to use all of C++.

**Games are script, not C++.**
*Why:* the development loop. Setting up a multi-platform C++ project take time and creating a new folder with a script file takes seconds. Rebuilding C++ can kill flow, while a VM reload is immediate.
*Costs:* A speed (10x) ceiling on gameplay, and debugging that crosses a boundary.

**The language is Wren.**
*Why:* class-based. Syntax close enough to the C# (and C++) family for a steady learning curve. Faster than Lua at a similar size and a VM small enough to comprehend (and modify). Milliseconds to compile to bytecode for a whole game. Native coroutines, nice for generator visualisation step-by-step.
*Costs:* smaller ecosystem than Lua. Wren is effectively unmaintained, so xs owns a fork.

**Gameplay lives in Wren, not the core (C++).** Entity-component, all components, containers, geometry helpers, all in script.
*Why:* keeps the core tiny and stateless (oblivious of gameplay). The makes live reload work
*Costs:* gameplay runs at script speed, which caps entity counts.


### The shape of the core

**D5 — Namespaces of free functions over module-local static state.**
`xs::render`, `xs::fileio`, `xs::audio`, each with `initialize`, `shutdown`, `update` and a
flat surface.
*Instead of:* an engine object graph, subsystems as classes, injection or a service
locator.
*Why:* minimal cognitive load. No lifetime to reason about, no wiring to trace, no question
which instance a call means. `render::sprite(...)` has exactly one place to look.
*Costs:* one instance per process, so xs can't be embedded twice or live inside a host.
Global state is harder to test in isolation. An engine is a process, and the samples are
the tests.

**D6 — Platform and backend variation is a compile-time question.**
`defines.hpp` derives feature macros from the platform macro; `platform.hpp` includes
exactly one platform header. No virtual backend interfaces.
*Instead of:* an abstract backend interface, implementation picked at runtime.
*Why:* a virtual interface turns one build-time question into a million runtime ones. Every
call site becomes a dispatch you can't resolve while reading. And the interface has to be
the union of everything every backend needs, including things only one platform has.
Compile-time means the code you're reading is the code that runs.
*Costs:* no runtime backend switching, and adding a platform means touching the switch
points instead of writing one class. `platforms/null/` exists so "this platform supplies
nothing" stays a valid answer.

**D7 — The script boundary carries primitives and opaque handles only.**
Images, sprites, shapes, fonts are ints. Colors are numbers. Vectors are Wren types.
*Instead of:* exposing engine objects to script.
*Why:* hides about 90% of the implementation, so the core can move without breaking
scripts. Also removes dangling references as a category — a script can't hold a pointer it
never had.
*Costs:* a less expressive, more verbose API, and type errors show up as bad handles rather
than at compile time.

**D8 — Rendering is deferred. Script fills a queue, the backend draws it once.**
*Instead of:* a GPU call per render call from Wren.
*Why:* crossing the script boundary is the expensive part, and a per-call path pays it with
nothing to batch. Deferring also keeps backends minimal — each one draws a queue and does
nothing else, which is what makes D6 affordable across six platforms.
*Costs:* a layer of indirection between submission and pixels, and draw order belongs to
the queue rather than to the code that filled it.




















































## Premises 

Not choices. Everything under **Decisions** is a choice made in response to these.

**One programmer's taste is baked in.** The author makes games by writing code, and the
games are procedural. Code-first isn't a conclusion xs reached; it's how its main user
already works. So where a decision below is justified by fitting that way of working,
that's a good reason for xs and no evidence it's right for anyone else. A designer-led
team reasoning the same way lands somewhere else.

**The work is procedural generation.** Write an algorithm, tweak a number, look, repeat.
The latency of that loop matters more than frame rate. Anything that lengthens it is
expensive even when it's free at runtime.

**One person has to hold all of it.** One developer builds and maintains xs and is its
main user. If it outgrows what he can keep in his head, it stops working for him. That's a
hard ceiling on size, and it settles most arguments.

**Capacity doesn't grow.** Staff and students help, but not on demand. Every subsystem
costs maintenance forever, not just once.

**Consoles are real.** Switch and PS5 are shipping targets, not someday. Bolting consoles
onto a desktop engine is close to a rewrite — folder layout, libraries, C++ version, build
system. So they're assumed from day one.

**Everything is text.** Code or JSON. No binary project state, no editor database.
Readable in an editor, diffable in git, legible to tools.

**Teaching is a use, not a constraint.** xs runs a course and a few masterclasses a year. A
student reading the whole engine in a term falls out of the size ceiling — it doesn't set
it. Worth protecting anyway.


## Goals

1. Small enough for one person to know all of. Core in the low thousands of lines.
2. Iteration in seconds. Script reload, asset reload, full C++ rebuild.
3. Code is the interface. The UI shows up when asked and otherwise stays away.
4. PCG is first class. Grids, shape building, noise, step-by-step visualisation.
5. One codebase, six platforms.
6. Clean checkout builds. No manual dependency staging.

---

## Not doing

A non-goal is a decision. Same treatment.

### 2D. Permanently.

**Instead of:** a 3D path. Tried on a branch.

**Why:** 3D didn't stay contained. It asked for asset pipelines, materials, scene
hierarchy, depth-correct rendering. Each reasonable alone, together several times the size
of the engine. That breaks the size ceiling.

**Costs:** a whole class of project can't use xs, and it can't follow a user into 3D. 2D is
what makes the rest affordable.

### No editor.

**Instead of:** a scene editor with a project database, like the engines most people
expect.

**Why:** an editor serves a workflow xs's users don't have. Content placed by hand is
content that can't be generated, and generation is the point. Second reason: an editor is
the biggest thing an engine can grow, and it needs upkeep every time the engine moves under
it. The inspector covers the real need — see and change values while the game runs.

**Costs:** authoring is less convenient, and some drag-and-drop jobs are code instead. This
is the decision that most narrows who xs is for. A project with an artist placing content
by hand will fight it.

### No API stability promise.

The API moves between versions. See **Versioning**.

### Not competing on breadth.

xs isn't measured against Unity, Unreal or Godot on feature count.

---

## Decisions

Each one: what, instead of what, why, what it costs.



### Simplicity

**D9 — Single-threaded.**
*Instead of:* a job system, or at least a render thread.
*Why:* simplicity, on purpose rather than by neglect. One sequential frame reads start to
finish, and a whole class of bug never exists. Nothing measured has shown the need.
*Costs:* idle cores, and a heavy generation step blocks the frame. Any threading proposal
has to argue against the size ceiling, not just show a speed-up.

**D10 — No exceptions.**
Levelled logging; asserts on broken invariants.
*Instead of:* exceptions for error propagation.
*Why:* not uniformly available or advisable across six platforms, and control flow you
can't see at the call site is the wrong default in code meant to be read.
*Costs:* errors are checked and propagated by hand. Some dependencies don't cooperate —
FMOD and the CLI parser — so the boundary isn't perfectly clean.

Script errors are a different thing and deliberately non-fatal: logged, flagged, session
survives. A typo in game code must not end the loop.

### Content and data

**D11 — One typed registry for engine config, tuning, saves and user settings.**
`Data.getNumber`, `getColor`, `getBool`, scoped by origin, JSON-backed.
*Instead of:* separate systems for settings, tuning and saves.
*Why:* one mechanism, one UI. Anything read through it is automatically live-editable.
That's the direct answer to the PCG premise: tuning a generator is changing a number and
looking, and here it costs one line of script and no engine change. Saves get cloud
persistence free where the platform has it.
*Costs:* a deliberately narrow type set, and one flat namespace per scope.

**D12 — All content goes through a wildcard path space.**
`[game]`, `[debug]`, `[save]`, `[user]`, resolved centrally in `fileio`.
*Instead of:* real paths, mode handled at each call site.
*Why:* the same path has to resolve against a project folder in development and a packaged
`.xs` when shipped. Central resolution means no loading code branches on run mode, and
packaging needs no change to game code. `[debug]` lets development-only content exist
without shipping.
*Costs:* they aren't real paths, so they can't be handed to a library doing its own file
I/O. FMOD is the friction.

### Platform reach

**D13 — Consoles from day one.**
*Instead of:* desktop first, consoles later.
*Why:* console support isn't additive. It constrains folder layout, library choice, C++
version and build system — cheap to get right at the start, expensive to revisit.
*Costs:* every subsystem is designed against the most restrictive platform, and some
convenient desktop libraries are out. Console code sits in NDA submodules, so part of the
engine isn't in the public repo.

**D14 — Native project files per platform.**
MSBuild on Windows and Switch, Xcode on Apple, CMake on Linux.
*Instead of:* one CMake project generating all of them.
*Why:* CMake doesn't cover the target set. iOS and both consoles need things it can't
express, so a single generator needs per-platform escapes anyway — at which point it adds a
layer without removing one.
*Costs:* build config in several places, and it can drift.

---

## How it fits together

Structure follows from the decisions above. It isn't separately motivated.

### Layers

Dependencies point down. Nothing in the portable core names a platform.

```
code/              engine core — portable C++17
  opengl/          OpenGL backend
  sdl3/            SDL3 device, input, audio
platforms/         entry points and native code
  pc/ linux/ apple/ null/ nx/ prospero/
external/          vendored dependencies
resources/modules/ Wren module library (D4)
samples/           examples, and the test suite
tools/             Python — version stamp, packaging, asset generation
```

| Layer | Contents |
| --- | --- |
| Game | Wren: a `Game` class with `initialize`, `update(dt)`, `render` |
| Wren library | `xs/ec`, `xs/components`, `xs/containers`, `xs/math` |
| Bindings | `script` — VM, binding registration, error state |
| Core | `render`, `audio`, `input`, `data`, `fileio`, `inspector`, `packager` |
| Backends | `opengl/`, `sdl3/`, Metal |
| Platform | entry point, windowing, paths, account |

The whole contract between engine and game is a Wren class named `Game` with three static
methods. A folder with a `project.json` and an entry `.wren` file is a runnable project.

### The split, in numbers

Core is ~8,400 lines of C++. The Wren library is ~2,500. The ratio is the point: anything
that can be Wren is Wren.

### The frame

1. `device::poll_events()`
2. `input::update(dt)` — outside the pause check, so the inspector stays live
3. If not paused: `render::clear()` → `script::update(dt)` → audio → `script::render()`
4. `device::begin_frame()`
5. `render::render()` — the queue is drawn (D8)
6. `inspector::render(dt)`
7. `device::end_frame()`

Load-bearing order: input before script, clear before submission, all submission before
render, inspector last so it draws over the game.

`dt` is clamped to 33 ms. Under 30 fps the simulation runs slow rather than taking big
steps — stability over real-time accuracy. A choice, not a side effect.

### Platforms and technology

D6 and D13, concretely. Each merged cell is one implementation covering several platforms.

![Platform and technology matrix](img/xs-platform-matrix.svg)

Six platforms, five varying concerns, one shared library set. The merges are where the
abstraction pays. The unmerged cells are what it costs.

### A project

<img src="img/project_folder.png" alt="An xs project folder" width="380">

A folder, a `project.json`, an entry script, assets, more JSON for game and debug data. No
generated project state, nothing binary, all of it diffable.

---

## Consequences

### Versioning

CalVer, `YY.BUILD`, where build is the commit count for the year, generated into
`code/version.hpp` at build time. Single source of version truth for the engine, the
installer and the package format.

Not semver, on purpose. D4 makes the Wren modules API, and they move. There's no
compatibility window to promise, so a major/minor/patch triple would claim a precision xs
can't deliver. Tooling that assumes semver semantics will be wrong.

### Risks

**The Wren fork.** D2 makes xs responsible for maintaining a language. It already diverges
and there's nothing upstream to merge from. If it grows it becomes a second engine.

**The modules are API with no stability story.** D4 makes them engine code and they change
between versions. Pinned projects are fine, projects tracking latest aren't. Nothing says
which modules are settled and which still move.

**Script-speed gameplay.** D3 caps entity counts and the cap hasn't been measured. Unknown
how close the samples run to it.

**Console code is invisible.** D13 puts part of the engine in NDA submodules. Outsiders
can't review it, students can't read it, and its maintainer pool is one person.

**Build drift.** D14 accepts several build definitions and nothing detects when one falls
behind.

### Open

- Notify on new version, self-update, or neither.
- Publish through a package manager, and which.
- What ends the alpha: a frozen subset of the Wren API, a stated compatibility window, or a
  deprecation policy.

---

## History

| Date | Change |
| --- | --- |
| 2026-09-02 | First consolidated version. |
