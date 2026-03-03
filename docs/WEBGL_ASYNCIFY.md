# WebGL Asyncify Architecture

This document explains how TRX's WebGL port handles the fundamental
incompatibility between a traditional C game loop and the browser's
single-threaded event model.

## The Problem

Browsers are **single-threaded**. The main thread handles JavaScript
execution, DOM events, and rendering. A traditional game loop blocks
indefinitely:

```c
while (running) {
    process_input();
    update();
    render();
    wait_for_next_frame();  // blocks the thread
}
```

Running this on the browser's main thread **freezes it entirely** — no
rendering, no input events, no page interaction. The browser never
regains control.

The standard browser solution is `requestAnimationFrame()`: you register
a callback the browser invokes once per frame, yielding control back
after each iteration. However, this requires **inverting the control
flow** — the browser owns the loop, not the game.

## Why Control Flow Inversion Breaks TRX

TRX has deeply nested, synchronous control flow. A simplified call chain
from startup to a single rendered frame:

```
main()
  Shell_Main()
    GF_DoFrontendSequence()
      GF_InterpretSequence()
        M_RunEvent()                    <- function pointer dispatch
          GF_RunGame() / FMV_Play()
            PhaseExecutor_Run()         <- blocking frame loop
              phase->start()            <- function pointer
              while (!done) {
                Clock_WaitTick()        <- blocks for frame timing
                phase->control()        <- function pointer
                phase->draw()           <- function pointer
              }
              phase->end()              <- function pointer
```

Key observations:

1. **`PhaseExecutor_Run` is a blocking loop.** It runs until the phase
   ends (could be an entire game level, a menu, or a cutscene).
2. **Game flow sequences are blocking calls.** `GF_DoFrontendSequence`
   doesn't return until the player exits the title screen.
3. **Function pointers are everywhere.** The phase executor dispatches
   to phase callbacks (`start`, `control`, `draw`, `end`) via function
   pointers. The game flow interpreter dispatches events the same way.
4. **Level loading is synchronous.** `Level_Pipeline_Load` reads and
   processes level data in a single blocking call.
5. **FMV playback blocks until done.** `FMV_Play` loops until the video
   ends or the player skips it.

To use `requestAnimationFrame()`, all of this would need to become a
flat state machine — the phase executor, the game flow interpreter,
level loading, FMV playback, everything. That is a ground-up rewrite of
the game's execution model.

## The Solution: Emscripten Asyncify

Instead of restructuring, the port uses **Asyncify**, an Emscripten
compiler feature that allows C code to yield to the browser without
blocking it.

When `emscripten_sleep(ms)` is called:

1. Asyncify **saves** the entire C call stack — all local variables,
   return addresses, and register state — into a side buffer.
2. It **returns** control to the browser event loop. The browser
   processes pending events, renders the page, handles input, etc.
3. After `ms` milliseconds, Asyncify **restores** the saved stack and
   resumes execution exactly where it left off.

From the C code's perspective, `emscripten_sleep()` behaves like a
normal blocking sleep. From the browser's perspective, the main thread
is free between yields. The game's synchronous architecture works
unmodified.

### Primary Yield Point

The main yield happens in `Clock_WaitTick()` (`game/clock/common.c`),
which calls `Platform_Yield()` — a thin wrapper around
`emscripten_sleep()` on WebGL and `SDL_Delay()` on desktop:

```c
// platform/yield.h
#ifdef EMSCRIPTEN_BUILD
static inline void Platform_Yield(unsigned int ms)
{
    emscripten_sleep(ms);
}
#else
static inline void Platform_Yield(unsigned int ms)
{
    if (ms > 0) {
        SDL_Delay((Uint32)ms);
    }
}
#endif
```

`Clock_WaitTick()` yields on every frame to maintain the target frame
rate, giving the browser a chance to run between each game frame.

### Additional Yield Points

Beyond frame timing, a few other operations yield to avoid freezing the
browser during long synchronous work:

| Location                    | Purpose                              |
|-----------------------------|--------------------------------------|
| `Clock_WaitTick()`          | Frame-paced timing (every frame)     |
| `Level_Pipeline_Load()`     | Yields before heavy I/O              |
| `Stats_CalculateMaxStats()` | Yields during level scanning         |
| `M_InitIDBFS()` (flow.c)   | Waits for IndexedDB sync             |
| `M_WaitForUserInput()`      | Start gate (browser audio policy)    |
| `FMV_Play()` (emscripten)   | Video frame sync loop                |

## The Complication: Function Pointers

Asyncify works by instrumenting compiled functions to save and restore
their stack frames. It can trace through **direct** function calls
automatically (it sees the call graph at compile time), but it
**cannot** trace through function pointer calls — the compiler doesn't
know which function will be called at runtime.

TRX's architecture uses function pointers extensively:
- The phase executor calls `phase->start`, `phase->control`,
  `phase->draw`, `phase->end`
- The game flow interpreter dispatches events through `M_RunEvent`
- Level loading triggers callbacks

If Asyncify doesn't know a function might yield, it won't instrument
it, and calling `emscripten_sleep()` from within that function causes
a runtime abort.

### ASYNCIFY_ADD Whitelist

The solution is the `-sASYNCIFY_ADD` linker flag, which explicitly lists
every function on the call path between `main()` and
`emscripten_sleep()` that crosses a function-pointer boundary. This is
configured in `src/meson.build`:

```
# ASYNCIFY cannot trace through function pointers (indirect calls).
# We must explicitly list every function on the call path between
# main() and emscripten_sleep(). The chain goes through multiple
# levels of function-pointer dispatch:
#
#   main -> Shell_Main -> GF_Do*Sequence -> GF_InterpretSequence
#   -> M_RunEvent -> event_handler [fnptr] -> GF_Run*/GF_Show*
#   -> PhaseExecutor_Run -> phase->start/control/draw [fnptr]
#   -> Clock_WaitTick -> emscripten_sleep
#
# Level loading also yields:
#   GF_InterpretSequence -> Level_Initialise -> Level_Pipeline_Load
#   -> emscripten_sleep
#
# FMV playback also yields:
#   M_HandlePlayFMV -> FMV_Play -> M_Play -> emscripten_sleep
```

The whitelist currently contains ~60 functions. Wildcards (`*M_Control*`,
`*M_Draw*`, etc.) match the per-phase implementations that get called
through function pointers.

Getting this list wrong causes either:
- **Missing function**: runtime abort when `emscripten_sleep()` is
  called from an uninstrumented call path
- **Too many functions**: unnecessarily large binary and slower
  execution (every instrumented function has save/restore overhead)

### Adding New Yield Points

When adding code that calls `Platform_Yield()` or `emscripten_sleep()`:

1. Trace the call path from `main()` to your new yield point.
2. Identify any function pointer dispatches along the path.
3. Add the relevant functions to `ASYNCIFY_ADD` in `src/meson.build`.
4. Test that the WebGL build doesn't abort at the new yield point.

If the new code is called only through direct function calls from
already-whitelisted paths, no changes to `ASYNCIFY_ADD` are needed —
Asyncify will instrument the new functions automatically.

## Build Configuration

The relevant Emscripten flags are set in `src/meson.build`:

| Flag                        | Value      | Purpose                              |
|-----------------------------|------------|--------------------------------------|
| `-sASYNCIFY`                | (enabled)  | Enable Asyncify stack switching      |
| `-sASYNCIFY_STACK_SIZE`     | 131072     | 128 KB save buffer for call stacks   |
| `-sASYNCIFY_ADD`            | (see above)| Whitelist for function-pointer paths  |
| `-sALLOW_MEMORY_GROWTH`     | 1          | Heap can grow beyond initial size    |
| `-sINITIAL_MEMORY`          | 268435456  | 256 MB initial heap                  |
| `-sSTACK_SIZE`              | 2097152    | 2 MB C stack                         |
| `-lidbfs.js`                | (linked)   | IndexedDB-backed persistent storage  |

## Why Not JSPI?

Emscripten also offers **JSPI** (JavaScript Promise Integration), a
newer alternative to Asyncify that uses the browser's native
promise-based stack switching. JSPI has lower overhead since it doesn't
need to copy the stack, but as of 2025 it requires Chrome 123+ with an
origin trial or flags, and Firefox/Safari support is incomplete. The
project uses classic Asyncify for maximum browser compatibility.
