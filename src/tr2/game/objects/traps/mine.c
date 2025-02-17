#include "game/objects/traps/mine.h"

#include "game/effects.h"
#include "game/items.h"
#include "game/objects/common.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/utils.h>

static int16_t M_GetBoatItem(const XYZ_32 *const pos, int16_t *room_num);
static void M_DetonateAll(
    const ITEM *mine_item, int16_t boat_item_num, int16_t boat_room_num);
static void M_Explode(ITEM *mine_item);

static int16_t M_GetBoatItem(const XYZ_32 *const pos, int16_t *const room_num)
{
    Room_GetSector(pos->x, pos->y, pos->z, room_num);

    int16_t item_num = Room_Get(*room_num)->item_num;
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        if (item->object_id == O_BOAT) {
            const int32_t dx = item->pos.x - pos->x;
            const int32_t dz = item->pos.z - pos->z;
            // TODO: fix overflows and no y check
            if (SQUARE(dx) + SQUARE(dz) < SQUARE(WALL_L / 2)) {
                break;
            }
        }
        item_num = item->next_item;
    }
    return item_num;
}

static void M_DetonateAll(
    const ITEM *const mine_item, const int16_t boat_item_num,
    int16_t boat_room_num)
{
    ITEM *const boat_item = Item_Get(boat_item_num);
    if (g_Lara.skidoo == boat_item_num) {
        Item_Explode(g_Lara.item_num, -1, 0);
        g_LaraItem->hit_points = 0;
        g_LaraItem->flags |= IF_ONE_SHOT;
    }

    boat_item->object_id = O_BOAT_BITS;
    Item_Explode(boat_item_num, -1, 0);
    Item_Kill(boat_item_num);
    boat_item->object_id = O_BOAT;

    Room_TestTriggers(mine_item);

    g_DetonateAllMines = true;
}

static void M_Explode(ITEM *const mine_item)
{
    const int16_t effect_num = Effect_Create(mine_item->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->object_id = O_EXPLOSION;
        effect->pos.x = mine_item->pos.x;
        effect->pos.y = mine_item->pos.y - WALL_L;
        effect->pos.z = mine_item->pos.z;
        effect->speed = 0;
        effect->frame_num = 0;
        effect->counter = 0;
    }

    Spawn_Splash(mine_item);
    Sound_Effect(SFX_EXPLOSION_1, &mine_item->pos, SPM_NORMAL);

    mine_item->flags |= IF_ONE_SHOT;
    mine_item->mesh_bits = 1;
    mine_item->collidable = 0;
}

void Mine_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->flags & IF_ONE_SHOT) {
        return;
    }

    if (!g_DetonateAllMines) {
        int16_t boat_room_num = item->room_num;
        XYZ_32 test_pos = {
            .x = item->pos.x,
            .y = item->pos.y - WALL_L * 2,
            .z = item->pos.z,
        };
        const int16_t boat_item_num = M_GetBoatItem(&test_pos, &boat_room_num);
        if (boat_item_num == NO_ITEM) {
            return;
        }

        M_DetonateAll(item, boat_item_num, boat_room_num);
    } else if (Random_GetControl() < 0x7800) {
        return;
    }

    M_Explode(item);
}

void Mine_Setup(void)
{
    OBJECT *const obj = Object_Get(O_MINE);
    obj->control_func = Mine_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
}
