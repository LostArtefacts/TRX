#include "game/collide.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/spawn.h"

#define TEETH_TRAP_DAMAGE 400

typedef enum {
    TEETH_TRAP_STATE_NICE = 0,
    TEETH_TRAP_STATE_NASTY = 1,
} TEETH_TRAP_STATE;

static BITE m_Teeth1A = { -23, 0, -1718, 0 };
static BITE m_Teeth1B = { 71, 0, -1718, 1 };
static BITE m_Teeth2A = { -23, 10, -1718, 0 };
static BITE m_Teeth2B = { 71, 10, -1718, 1 };
static BITE m_Teeth3A = { -23, -10, -1718, 0 };
static BITE m_Teeth3B = { 71, -10, -1718, 1 };

static void M_BiteEffect(ITEM *item, BITE *bite);
static void M_Setup(OBJECT *obj);
static void M_Control(int16_t item_num);

static void M_BiteEffect(ITEM *item, BITE *bite)
{
    XYZ_32 pos = {
        .x = bite->x,
        .y = bite->y,
        .z = bite->z,
    };
    Collide_GetJointAbsPosition(item, &pos, bite->mesh_num);
    Spawn_Blood(pos.x, pos.y, pos.z, item->speed, item->rot.y, item->room_num);
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = Object_CollisionTrap;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        item->goal_anim_state = TEETH_TRAP_STATE_NASTY;
        if (item->touch_bits
            && item->current_anim_state == TEETH_TRAP_STATE_NASTY) {
            Lara_TakeDamage(TEETH_TRAP_DAMAGE, true);
            M_BiteEffect(item, &m_Teeth1A);
            M_BiteEffect(item, &m_Teeth1B);
            M_BiteEffect(item, &m_Teeth2A);
            M_BiteEffect(item, &m_Teeth2B);
            M_BiteEffect(item, &m_Teeth3A);
            M_BiteEffect(item, &m_Teeth3B);
        }
    } else {
        item->goal_anim_state = TEETH_TRAP_STATE_NICE;
    }
    Item_Animate(item);
}

REGISTER_OBJECT(O_TEETH_TRAP, M_Setup)
