#include "game/collide.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/spawn.h"

#define TEETH_TRAP_DAMAGE 400

typedef enum {
    TEETH_TRAP_STATE_NICE = 0,
    TEETH_TRAP_STATE_NASTY = 1,
} TEETH_TRAP_STATE;

static const BITE m_Teeth[6] = {
    // clang-format off
    { .pos = { .x = -23, .y = 0,   .z = -1718 }, .mesh_num = 0 },
    { .pos = { .x = 71,  .y = 0,   .z = -1718 }, .mesh_num = 1 },
    { .pos = { .x = -23, .y = 10,  .z = -1718 }, .mesh_num = 0 },
    { .pos = { .x = 71,  .y = 10,  .z = -1718 }, .mesh_num = 1 },
    { .pos = { .x = -23, .y = -10, .z = -1718 }, .mesh_num = 0 },
    { .pos = { .x = 71,  .y = -10, .z = -1718 }, .mesh_num = 1 },
    // clang-format on
};

static void M_Bite(ITEM *item, const BITE *bite);
static void M_Control(int16_t item_num);
static void M_Setup(OBJECT *obj);

static void M_Bite(ITEM *const item, const BITE *const bite)
{
    XYZ_32 pos = bite->pos;
    Collide_GetJointAbsPosition(item, &pos, bite->mesh_num);
    Spawn_Blood(pos.x, pos.y, pos.z, item->speed, item->rot.y, item->room_num);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)) {
        item->goal_anim_state = TEETH_TRAP_STATE_NASTY;
        if (item->touch_bits != 0
            && item->current_anim_state == TEETH_TRAP_STATE_NASTY) {
            Lara_TakeDamage(TEETH_TRAP_DAMAGE, true);
            M_Bite(item, &m_Teeth[0]);
            M_Bite(item, &m_Teeth[1]);
            M_Bite(item, &m_Teeth[2]);
            M_Bite(item, &m_Teeth[3]);
            M_Bite(item, &m_Teeth[4]);
            M_Bite(item, &m_Teeth[5]);
        }
    } else {
        item->goal_anim_state = TEETH_TRAP_STATE_NICE;
    }

    Item_Animate(item);
}

REGISTER_OBJECT(O_TEETH_TRAP, M_Setup)
