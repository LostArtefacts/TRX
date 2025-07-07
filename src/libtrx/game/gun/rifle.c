#include "game/gun/rifle.h"

#include "game/gun/common.h"

void Gun_Rifle_DrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHandRMesh(weapon_type);
    Gun_SetLaraBackMesh(LGT_UNARMED);
}

void Gun_Rifle_UndrawMeshes(const LARA_GUN_TYPE weapon_type)
{
    Gun_SetLaraHandLMesh(LGT_UNARMED);
    Gun_SetLaraHandRMesh(LGT_UNARMED);
    Gun_SetLaraBackMesh(weapon_type);
}
