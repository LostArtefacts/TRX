#include "game/lara/lara_cheat.h"

#include "game/carrier.h"
#include "game/console.h"
#include "game/effects/exploding_death.h"
#include "game/game_string.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void Lara_Cheat_Control(void)
{
    static int32_t cheat_mode = 0;
    static int16_t cheat_angle = 0;
    static int32_t cheat_turn = 0;

    if (g_CurrentLevel == g_GameFlow.gym_level_num) {
        return;
    }

    LARA_STATE as = g_LaraItem->current_anim_state;
    switch (cheat_mode) {
    case 0:
        if (as == LS_WALK) {
            cheat_mode = 1;
        }
        break;

    case 1:
        if (as != LS_WALK) {
            cheat_mode = as == LS_STOP ? 2 : 0;
        }
        break;

    case 2:
        if (as != LS_STOP) {
            cheat_mode = as == LS_BACK ? 3 : 0;
        }
        break;

    case 3:
        if (as != LS_BACK) {
            cheat_mode = as == LS_STOP ? 4 : 0;
        }
        break;

    case 4:
        if (as != LS_STOP) {
            cheat_angle = g_LaraItem->rot.y;
        }
        cheat_turn = 0;
        if (as == LS_TURN_L) {
            cheat_mode = 5;
        } else if (as == LS_TURN_R) {
            cheat_mode = 6;
        } else {
            cheat_mode = 0;
        }
        break;

    case 5:
        if (as == LS_TURN_L || as == LS_FAST_TURN) {
            cheat_turn += (int16_t)(g_LaraItem->rot.y - cheat_angle);
            cheat_angle = g_LaraItem->rot.y;
        } else {
            cheat_mode = cheat_turn < -94208 ? 7 : 0;
        }
        break;

    case 6:
        if (as == LS_TURN_R || as == LS_FAST_TURN) {
            cheat_turn += (int16_t)(g_LaraItem->rot.y - cheat_angle);
            cheat_angle = g_LaraItem->rot.y;
        } else {
            cheat_mode = cheat_turn > 94208 ? 7 : 0;
        }
        break;

    case 7:
        if (as != LS_STOP) {
            cheat_mode = as == LS_COMPRESS ? 8 : 0;
        }
        break;

    case 8:
        if (g_LaraItem->fall_speed > 0) {
            if (as == LS_JUMP_FORWARD) {
                g_LevelComplete = true;
            } else if (as == LS_JUMP_BACK) {
                Inv_AddItem(O_SHOTGUN_ITEM);
                Inv_AddItem(O_MAGNUM_ITEM);
                Inv_AddItem(O_UZI_ITEM);
                g_Lara.shotgun.ammo = 500;
                g_Lara.magnums.ammo = 500;
                g_Lara.uzis.ammo = 5000;
                Sound_Effect(SFX_LARA_HOLSTER, NULL, SPM_ALWAYS);
            } else if (as == LS_SWAN_DIVE) {
                Effect_ExplodingDeath(g_Lara.item_number, -1, 0);
                Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
                g_LaraItem->hit_points = 0;
                g_LaraItem->flags |= IS_INVISIBLE;
            }
            cheat_mode = 0;
        }
        break;

    default:
        cheat_mode = 0;
        break;
    }
}

void Lara_Cheat_EndLevel(void)
{
    g_LevelComplete = true;
    Console_Log(GS(OSD_COMPLETE_LEVEL));
}

bool Lara_Cheat_EnterFlyMode(void)
{
    if (g_LaraItem == NULL) {
        return false;
    }

    if (g_Lara.water_status != LWS_UNDERWATER || g_LaraItem->hit_points <= 0) {
        g_LaraItem->pos.y -= 0x80;
        g_LaraItem->current_anim_state = LS_SWIM;
        g_LaraItem->goal_anim_state = LS_SWIM;
        Item_SwitchToAnim(g_LaraItem, LA_SWIM_GLIDE, 0);
        g_LaraItem->gravity_status = 0;
        g_LaraItem->rot.x = 30 * PHD_DEGREE;
        g_LaraItem->fall_speed = 30;
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }
    g_Lara.water_status = LWS_CHEAT;
    g_Lara.spaz_effect_count = 0;
    g_Lara.spaz_effect = NULL;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = -1;
    g_Lara.air = LARA_AIR;
    g_Lara.death_timer = 0;
    g_Lara.mesh_effects = 0;
    Lara_InitialiseMeshes(g_CurrentLevel);
    g_Camera.type = CAM_CHASE;
    Viewport_SetFOV(Viewport_GetUserFOV());
    Console_Log(GS(OSD_FLY_MODE_ON));
    return true;
}

bool Lara_Cheat_ExitFlyMode(void)
{
    if (g_LaraItem == NULL) {
        return false;
    }

    const ROOM_INFO *const room = &g_RoomInfo[g_LaraItem->room_number];
    const bool room_submerged = (room->flags & RF_UNDERWATER) != 0;
    const int16_t water_height = Room_GetWaterHeight(
        g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z,
        g_LaraItem->room_number);

    if (room_submerged || (water_height != NO_HEIGHT && water_height > 0)) {
        g_Lara.water_status = LWS_UNDERWATER;
    } else {
        g_Lara.water_status = LWS_ABOVE_WATER;
        Item_SwitchToAnim(g_LaraItem, LA_STOP, 0);
        g_LaraItem->rot.x = 0;
        g_LaraItem->rot.z = 0;
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }
    g_Lara.gun_status = LGS_ARMLESS;
    Console_Log(GS(OSD_FLY_MODE_OFF));
    return true;
}

bool Lara_Cheat_GiveAllKeys(void)
{
    if (g_LaraItem == NULL) {
        return false;
    }

    Inv_AddItem(O_PUZZLE_ITEM1);
    Inv_AddItem(O_PUZZLE_ITEM2);
    Inv_AddItem(O_PUZZLE_ITEM3);
    Inv_AddItem(O_PUZZLE_ITEM4);
    Inv_AddItem(O_KEY_ITEM1);
    Inv_AddItem(O_KEY_ITEM2);
    Inv_AddItem(O_KEY_ITEM3);
    Inv_AddItem(O_KEY_ITEM4);
    Inv_AddItem(O_PICKUP_ITEM1);
    Inv_AddItem(O_PICKUP_ITEM2);

    Sound_Effect(SFX_LARA_KEY, NULL, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_KEYS));
    return true;
}

bool Lara_Cheat_GiveAllGuns(void)
{
    if (g_LaraItem == NULL) {
        return false;
    }

    Inv_AddItem(O_GUN_ITEM);
    Inv_AddItem(O_MAGNUM_ITEM);
    Inv_AddItem(O_UZI_ITEM);
    Inv_AddItem(O_SHOTGUN_ITEM);
    g_Lara.shotgun.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 300;
    g_Lara.magnums.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 1000;
    g_Lara.uzis.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 2000;

    Sound_Effect(SFX_LARA_RELOAD, NULL, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_GUNS));
    return true;
}

bool Lara_Cheat_GiveAllItems(void)
{
    if (g_LaraItem == NULL) {
        return false;
    }

    Inv_AddItem(O_GUN_ITEM);

    if (!Inv_RequestItem(O_SHOTGUN_ITEM)) {
        Inv_AddItem(O_SHOTGUN_ITEM);
    }
    g_Lara.shotgun.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 300;

    if (!Inv_RequestItem(O_MAGNUM_ITEM)) {
        Inv_AddItem(O_MAGNUM_ITEM);
    }
    g_Lara.magnums.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 1000;

    if (!Inv_RequestItem(O_UZI_ITEM)) {
        Inv_AddItem(O_UZI_ITEM);
    }
    g_Lara.uzis.ammo = g_GameInfo.bonus_flag & GBF_NGPLUS ? 10001 : 2000;

    for (int i = 0; i < 10; i++) {
        if (Inv_RequestItem(O_MEDI_ITEM) < 240) {
            Inv_AddItem(O_MEDI_ITEM);
        }
        if (Inv_RequestItem(O_BIGMEDI_ITEM) < 240) {
            Inv_AddItem(O_BIGMEDI_ITEM);
        }
    }

    if (!Inv_RequestItem(O_KEY_ITEM1)) {
        Inv_AddItem(O_KEY_ITEM1);
    }
    if (!Inv_RequestItem(O_KEY_ITEM2)) {
        Inv_AddItem(O_KEY_ITEM2);
    }
    if (!Inv_RequestItem(O_KEY_ITEM3)) {
        Inv_AddItem(O_KEY_ITEM3);
    }
    if (!Inv_RequestItem(O_KEY_ITEM4)) {
        Inv_AddItem(O_KEY_ITEM4);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM1)) {
        Inv_AddItem(O_PUZZLE_ITEM1);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM2)) {
        Inv_AddItem(O_PUZZLE_ITEM2);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM3)) {
        Inv_AddItem(O_PUZZLE_ITEM3);
    }
    if (!Inv_RequestItem(O_PUZZLE_ITEM4)) {
        Inv_AddItem(O_PUZZLE_ITEM4);
    }
    if (!Inv_RequestItem(O_PICKUP_ITEM1)) {
        Inv_AddItem(O_PICKUP_ITEM1);
    }
    if (!Inv_RequestItem(O_PICKUP_ITEM2)) {
        Inv_AddItem(O_PICKUP_ITEM2);
    }

    Sound_Effect(SFX_LARA_HOLSTER, &g_LaraItem->pos, SPM_NORMAL);
    Console_Log(GS(OSD_GIVE_ITEM_CHEAT));
    return true;
}

bool Lara_Cheat_OpenNearestDoor(void)
{
    if (g_LaraItem == NULL) {
        return false;
    }

    int32_t opened = 0;
    int32_t closed = 0;

    const int32_t shift = 8; // constant shift to avoid overflow errors
    const int32_t max_dist = SQUARE((WALL_L * 2) >> shift);
    for (int item_num = 0; item_num < g_LevelItemCount; item_num++) {
        ITEM_INFO *const item = &g_Items[item_num];
        if (!Object_IsObjectType(item->object_number, g_DoorObjects)
            && !Object_IsObjectType(item->object_number, g_TrapdoorObjects)) {
            continue;
        }

        const int32_t dx = (item->pos.x - g_LaraItem->pos.x) >> shift;
        const int32_t dy = (item->pos.y - g_LaraItem->pos.y) >> shift;
        const int32_t dz = (item->pos.z - g_LaraItem->pos.z) >> shift;
        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > max_dist) {
            continue;
        }

        if (!item->active) {
            Item_AddActive(item_num);
            item->flags |= IF_CODE_BITS;
            opened++;
        } else if (item->flags & IF_CODE_BITS) {
            item->flags &= ~IF_CODE_BITS;
            closed++;
        } else {
            item->flags |= IF_CODE_BITS;
            opened++;
        }
        item->timer = 0;
        item->touch_bits = 0;
    }

    if (opened > 0 || closed > 0) {
        Console_Log(opened > 0 ? GS(OSD_DOOR_OPEN) : GS(OSD_DOOR_CLOSE));
        return true;
    }
    Console_Log(GS(OSD_DOOR_OPEN_FAIL));
    return false;
}

bool Lara_Cheat_KillEnemy(const int16_t item_num)
{
    struct ITEM_INFO *item = &g_Items[item_num];
    if (!Object_IsObjectType(item->object_number, g_EnemyObjects)
        || item->hit_points <= 0) {
        return false;
    }

    Effect_ExplodingDeath(item_num, -1, 0);
    Sound_Effect(SFX_EXPLOSION_CHEAT, &item->pos, SPM_NORMAL);
    Item_Kill(item_num);
    Carrier_TestItemDrops(item_num);
    return true;
}
