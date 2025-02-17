#include "game/objects/traps/dying_monk.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/game_buf.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/utils.h>

#define MAX_ROOMIES 2

void DyingMonk_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    int32_t *const roomies =
        GameBuf_Alloc(sizeof(int32_t) * MAX_ROOMIES, GBUF_ITEM_DATA);
    for (int32_t i = 0; i < MAX_ROOMIES; i++) {
        roomies[i] = NO_ITEM;
    }

    int32_t roomie_count = 0;
    int16_t test_item_num = Room_Get(item->room_num)->item_num;
    while (test_item_num != NO_ITEM) {
        const ITEM *const test_item = Item_Get(test_item_num);
        const OBJECT *const test_obj = Object_Get(test_item->object_id);
        if (test_obj->intelligent) {
            roomies[roomie_count++] = test_item_num;
            if (roomie_count >= MAX_ROOMIES) {
                break;
            }
        }
        test_item_num = test_item->next_item;
    }

    item->data = roomies;
}

void DyingMonk_Control(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    const int32_t *const roomies = item->data;

    for (int32_t i = 0; i < MAX_ROOMIES; i++) {
        int32_t test_item_num = roomies[i];
        if (test_item_num != NO_ITEM) {
            const ITEM *const test_item = Item_Get(test_item_num);
            if (test_item->hit_points > 0) {
                return;
            }
        }
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = ABS(lara_item->pos.x - item->pos.x);
    const int32_t dz = ABS(lara_item->pos.z - item->pos.z);
    if (dx < WALL_L && dz < WALL_L && !lara_item->gravity
        && lara_item->pos.y == item->pos.y) {
        g_LevelComplete = true;
    }
}

void DyingMonk_Setup(void)
{
    OBJECT *const obj = Object_Get(O_DYING_MONK);
    obj->initialise_func = DyingMonk_Initialise;
    obj->control_func = DyingMonk_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
}
