#include "game/objects/traps/teeth_trap.h"

#include "game/collide.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/common.h"

#include <stdbool.h>

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

static void TeethTrap_BiteEffect(ITEM_INFO *item, BITE_INFO *bite);

static void TeethTrap_BiteEffect(ITEM_INFO *item, BITE_INFO *bite)
{
    PHD_VECTOR pos;
    pos.x = bite->x;
    pos.y = bite->y;
    pos.z = bite->z;
    Collide_GetJointAbsPosition(item, &pos, bite->mesh_num);
    Effect_Blood(
        pos.x, pos.y, pos.z, item->speed, item->pos.y_rot, item->room_number);
}

void TeethTrap_Setup(OBJECT_INFO *obj)
{
    obj->control = TeethTrap_Control;
    obj->collision = Object_CollisionTrap;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

void TeethTrap_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (Item_IsTriggerActive(item)) {
        item->goal_anim_state = TT_NASTY;
        if (item->touch_bits && item->current_anim_state == TT_NASTY) {
            Lara_TakeDamage(TEETH_TRAP_DAMAGE, true);
            TeethTrap_BiteEffect(item, &m_Teeth1A);
            TeethTrap_BiteEffect(item, &m_Teeth1B);
            TeethTrap_BiteEffect(item, &m_Teeth2A);
            TeethTrap_BiteEffect(item, &m_Teeth2B);
            TeethTrap_BiteEffect(item, &m_Teeth3A);
            TeethTrap_BiteEffect(item, &m_Teeth3B);
        }
    } else {
        item->goal_anim_state = TT_NICE;
    }
    Item_Animate(item);
}
