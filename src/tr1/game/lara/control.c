#include "game/lara/control.h"

#include "game/gun.h"
#include "game/input.h"
#include "game/lara/common.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>

#include <stdint.h>

static int32_t m_OpenDoorsCheatCooldown = 0;

static SECTOR *M_GetCurrentSector(const ITEM *lara_item);

static SECTOR *M_GetCurrentSector(const ITEM *const lara_item)
{
    int16_t room_num = lara_item->room_num;
    return Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
}

void Lara_HandleUnderwater(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -LARA_HEIGHT_UW;
    coll->bad_ceiling = LARA_HEIGHT_UW;
    coll->old.x = item->pos.x;
    coll->old.y = item->pos.y;
    coll->old.z = item->pos.z;
    coll->radius = LARA_RADIUS_UW;
    coll->slopes_are_walls = 0;
    coll->slopes_are_pits = 0;
    coll->lava_is_pit = 0;
    coll->enable_hit = 0;
    coll->enable_baddie_push = 0;

    Lara_Look_Update();
    Lara_State_Update(item, coll);

    if (item->rot.z >= -(2 * LARA_LEAN_UNDO)
        && item->rot.z <= 2 * LARA_LEAN_UNDO) {
        item->rot.z = 0;
    } else if (item->rot.z < 0) {
        item->rot.z += 2 * LARA_LEAN_UNDO;
    } else {
        item->rot.z -= 2 * LARA_LEAN_UNDO;
    }

    if (g_Config.gameplay.enable_tr2_swimming) {
        CLAMP(item->rot.x, -85 * DEG_1, 85 * DEG_1);
        CLAMP(item->rot.z, -LARA_LEAN_MAX_UW, LARA_LEAN_MAX_UW);

        if (g_Lara.turn_rate < -LARA_TURN_UNDO) {
            g_Lara.turn_rate += LARA_TURN_UNDO;
        } else if (g_Lara.turn_rate > LARA_TURN_UNDO) {
            g_Lara.turn_rate -= LARA_TURN_UNDO;
        } else {
            g_Lara.turn_rate = 0;
        }
        item->rot.y += g_Lara.turn_rate;
    } else {
        CLAMP(item->rot.x, -100 * DEG_1, 100 * DEG_1);
        CLAMP(item->rot.z, -LARA_LEAN_MAX_UW, LARA_LEAN_MAX_UW);
    }

    if (g_Lara.current_active && g_Lara.water_status != LWS_CHEAT) {
        Lara_WaterCurrent(coll);
    } else {
        LOT_ClearLOT(&g_Lara.lot);
    }

    Lara_Animate(item);

    item->pos.y -=
        (Math_Sin(item->rot.x) * item->fall_speed) >> (W2V_SHIFT + 2);
    item->pos.x +=
        (((Math_Sin(item->rot.y) * item->fall_speed) >> (W2V_SHIFT + 2))
         * Math_Cos(item->rot.x))
        >> W2V_SHIFT;
    item->pos.z +=
        (((Math_Cos(item->rot.y) * item->fall_speed) >> (W2V_SHIFT + 2))
         * Math_Cos(item->rot.x))
        >> W2V_SHIFT;

    const SECTOR *const sector = M_GetCurrentSector(item);

    if (g_Lara.water_status != LWS_CHEAT) {
        Lara_BaddieCollision(item, coll);
    }

    if (g_Lara.water_status == LWS_CHEAT) {
        if (m_OpenDoorsCheatCooldown) {
            m_OpenDoorsCheatCooldown--;
        } else if (g_Input.draw) {
            m_OpenDoorsCheatCooldown = LOGIC_FPS;
            Lara_Cheat_OpenNearestDoor();
        }
    }

    Lara_Col_Update(item, coll);
    Lara_UpdateRoomToHeight(0);
    Gun_Control();
    Room_TestSectorTrigger(item, sector);
}
