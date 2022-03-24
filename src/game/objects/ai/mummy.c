#include "game/objects/ai/mummy.h"

#include "3dsystem/phd_math.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/gamebuf.h"
#include "game/items.h"
#include "global/vars.h"

void SetupMummy(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseMummy;
    obj->control = MummyControl;
    obj->collision = ObjectCollision;
    obj->hit_points = MUMMY_HITPOINTS;
    obj->save_flags = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    g_AnimBones[obj->bone_index + 8] |= BEB_ROT_Y;
}

void InitialiseMummy(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    item->touch_bits = 0;
    item->mesh_bits = 0xFFFF87FF;
    item->data = GameBuf_Alloc(sizeof(int16_t), GBUF_CREATURE_DATA);
    *(int16_t *)item->data = 0;
}

void MummyControl(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    int16_t head = 0;

    if (item->current_anim_state == MUMMY_STOP) {
        head = phd_atan(
                   g_LaraItem->pos.z - item->pos.z,
                   g_LaraItem->pos.x - item->pos.x)
            - item->pos.y_rot;
        CLAMP(head, -FRONT_ARC, FRONT_ARC);

        if (item->hit_points <= 0 || item->touch_bits) {
            item->goal_anim_state = MUMMY_DEATH;
        }
    }

    CreatureHead(item, head);
    AnimateItem(item);

    if (item->status == IS_DEACTIVATED) {
        RemoveActiveItem(item_num);
        item->hit_points = DONT_TARGET;
    }
}
