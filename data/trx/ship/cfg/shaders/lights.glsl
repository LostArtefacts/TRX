#include "lights_common.glsl"

#if TR_VERSION >= 4
    #include "lights_tr4.glsl"
#elif TR_VERSION >= 3
    #include "lights_tr3.glsl"
#else
    #include "lights_tr12.glsl"
#endif
