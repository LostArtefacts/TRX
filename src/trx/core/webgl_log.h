#pragma once

// Diagnostic logging macro for WebGL/Emscripten builds.
// On Emscripten, writes directly to the browser console via emscripten_log().
// On desktop, compiles to a no-op.

#ifdef EMSCRIPTEN_BUILD
    #include <emscripten.h>
    #define WEBGL_LOG(...) emscripten_log(0x02, __VA_ARGS__)
#else
    #define WEBGL_LOG(...) ((void)0)
#endif
