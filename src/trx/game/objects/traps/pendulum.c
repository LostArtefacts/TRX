#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#define M_AXE_DAMAGE 100
#define M_PENDULUM_DAMAGE 50

static inline int16_t M_GetDamage(const OBJECT_ID obj_id)
{
    return obj_id == O_SWINGING_AXE ? M_AXE_DAMAGE : M_PENDULUM_DAMAGE;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    const bool working =
        g_TRVersion != 1 || item->current_anim_state == TRAP_WORKING;
    if (g_TRVersion == 1) {
        if (Item_IsTriggerActive(item)) {
            if (item->current_anim_state == TRAP_SET) {
                item->goal_anim_state = TRAP_WORKING;
            }
        } else {
            if (item->current_anim_state == TRAP_WORKING) {
                item->goal_anim_state = TRAP_SET;
            }
        }
    }

    if (working && item->touch_bits != 0) {
        const int16_t damage = M_GetDamage(item->object_id);
        Lara_TakeDamage(damage, true);

        const ITEM *const lara_item = Lara_GetItem();
        const XYZ_32 pos = {
            .x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256,
            .z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256,
            .y = lara_item->pos.y - Random_GetControl() / 44,
        };
        Spawn_Blood(
            pos.x, pos.y, pos.z, lara_item->speed,
            lara_item->rot.y + (Random_GetControl() - 0x4000) / 8,
            lara_item->room_num);
    }

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &item->room_num);
    item->floor = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    Item_Animate(item);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupAxe(OBJECT *const obj)
{
    M_SetupCommon(obj);
    obj->collision_func = Object_Collision_Trap;
}

static void M_SetupPendulum(OBJECT *const obj)
{
    M_SetupCommon(obj);
    obj->collision_func = Object_Collision;
}

REGISTER_OBJECT(O_SWINGING_AXE, M_SetupAxe)
REGISTER_OBJECT(O_PENDULUM_1, M_SetupPendulum)
REGISTER_OBJECT(O_PENDULUM_2, M_SetupPendulum)
