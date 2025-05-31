#include "game/lara/flare.h"

#include "game/lara.h"

bool Lara_Flare_IsMeshActive(void)
{
#if TR_VERSION == 1
    return false;
#else
    const OBJECT *const obj = Object_Get(O_LARA_FLARE);
    return obj->loaded
        && Lara_GetMesh(LM_HAND_L) == Object_GetMesh(obj->mesh_idx + LM_HAND_L);
#endif
}

void Lara_Flare_DrawMeshes(void)
{
#if TR_VERSION >= 2
    Lara_SwapSingleMesh(LM_HAND_L, O_LARA_FLARE);
#endif
}
