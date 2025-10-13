#include "game/lara/flare.h"

#include "game/lara.h"

bool Lara_Flare_IsMeshActive(void)
{
    const OBJECT *const obj = Object_Get(O_LARA_FLARE);
    return obj->loaded
        && Lara_Mesh_Get(LM_HAND_L)
        == Object_GetMesh(obj->mesh_idx + LM_HAND_L);
}

void Lara_Flare_DrawMeshes(void)
{
    Lara_Mesh_SwapSingle(LM_HAND_L, O_LARA_FLARE);
}
