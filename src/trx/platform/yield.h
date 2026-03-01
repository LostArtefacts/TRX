#pragma once

// Platform-aware yield / delay function.
//
// On Emscripten, calls emscripten_sleep() so ASYNCIFY yields to the browser
// event loop. On desktop, calls SDL_Delay() for non-zero delays and is a
// no-op for zero-delay yields (desktop does not need to yield).

#ifdef EMSCRIPTEN_BUILD
    #include <emscripten.h>
static inline void Platform_Yield(unsigned int ms)
{
    emscripten_sleep(ms);
}
#else
    #include <SDL2/SDL_timer.h>
static inline void Platform_Yield(unsigned int ms)
{
    if (ms > 0) {
        SDL_Delay((Uint32)ms);
    }
}
#endif
