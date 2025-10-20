#include "game/item_actions/lara_effects.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/viewport.h>
#include <libtrx/utils.h>

void ItemAction_LaraHandsFree(ITEM *item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->gun_status = LGS_ARMLESS;
}

void ItemAction_LaraDrawRightGun(ITEM *item)
{
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_THIGH_R);
    Object_SwapMesh(item->object_id, O_LARA_PISTOLS, LM_HAND_R);
}
