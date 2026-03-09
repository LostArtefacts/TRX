#pragma once

#ifdef EMSCRIPTEN_BUILD
    #include <trx/av/video_emscripten.h>
#else
    #include <trx/av/video_generic.h>
#endif
