#include "game/traps/teeth_trap.h"

#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/sphere.h"
#include "global/vars.h"

#define TEETH_TRAP_DAMAGE 400

typedef enum {
    TT_NICE = 0,
    TT_NASTY = 1,
} TEETH_TRAP_STATE;

static BITE_INFO m_Teeth1A = { -23, 0, -1718, 0 };
static BITE_INFO m_Teeth1B = { 71, 0, -1718, 1 };
static BITE_INFO m_Teeth2A = { -23, 10, -1718, 0 };
static BITE_INFO m_Teeth2B = { 71, 10, -1718, 1 };
static BITE_INFO m_Teeth3A = { -23, -10, -1718, 0 };
static BITE_INFO m_Teeth3B = { 71, -10, -1718, 1 };

static void BaddieBiteEffect(ITEM_INFO *item, BITE_INFO *bite);

static void BaddieBiteEffect(ITEM_INFO *item, BITE_INFO *bite)
{
    PHD_VECTOR pos;
    pos.x = bite->x;
    pos.y = bite->y;
    pos.z = bite->z;
    GetJointAbsPosition(item, &pos, bite->mesh_num);
    DoBloodSplat(
        pos.x, pos.y, pos.z, item->speed, item->pos.y_rot, item->room_number);
}

void TeethTrap_Setup(OBJECT_INFO *obj)
{
    obj->control = TeethTrap_Control;
    obj->collision = TrapCollision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void TeethTrap_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (TriggerActive(item)) {
        item->goal_anim_state = TT_NASTY;
        if (item->touch_bits && item->current_anim_state == TT_NASTY) {
            g_LaraItem->hit_points -= TEETH_TRAP_DAMAGE;
            g_LaraItem->hit_status = 1;
            BaddieBiteEffect(item, &m_Teeth1A);
            BaddieBiteEffect(item, &m_Teeth1B);
            BaddieBiteEffect(item, &m_Teeth2A);
            BaddieBiteEffect(item, &m_Teeth2B);
            BaddieBiteEffect(item, &m_Teeth3A);
            BaddieBiteEffect(item, &m_Teeth3B);
        }
    } else {
        item->goal_anim_state = TT_NICE;
    }
    AnimateItem(item);
}
