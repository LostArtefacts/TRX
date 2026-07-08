#include "lights_common.glsl"

// TODO: dedicated TR4 lighting family; rides the TR3 family for now.
#if TR_VERSION >= 3
    #include "lights_tr3.glsl"
#else
    #include "lights_tr12.glsl"
#endif
