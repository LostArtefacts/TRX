#include "game/objects/general/door.h"

#include "game/box.h"
#include "game/items.h"
#include "game/lara/misc.h"
#include "global/funcs.h"
#include "global/vars.h"

void __cdecl Door_Shut(DOORPOS_DATA *const d)
{
    SECTOR *const sector = d->sector;
    if (d->sector == NULL) {
        return;
    }

    sector->idx = 0;
    sector->box = NO_BOX;
    sector->ceiling = NO_HEIGHT / STEP_L;
    sector->floor = NO_HEIGHT / STEP_L;
    sector->sky_room = NO_ROOM_NEG;
    sector->pit_room = NO_ROOM_NEG;

    const int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index |= BOX_BLOCKED;
    }
}

void __cdecl Door_Open(DOORPOS_DATA *const d)
{
    if (d->sector == NULL) {
        return;
    }

    *d->sector = d->old_sector;

    const int16_t box_num = d->block;
    if (box_num != NO_BOX) {
        g_Boxes[box_num].overlap_index &= ~BOX_BLOCKED;
    }
}

void __cdecl Door_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = &g_Items[item_num];

    if (!Item_TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }

    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(
            item, lara_item, coll,
            item->current_anim_state != item->goal_anim_state
                ? coll->enable_spaz
                : false,
            true);
    }
}
