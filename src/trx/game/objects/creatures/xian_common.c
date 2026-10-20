#include <trx/game/objects/creatures/xian_common.h>

#include <trx/game/items/draw.h>
#include <trx/game/objects/common.h>

bool XianWarrior_Draw(const ITEM *item)
{
    const OBJECT *swap;
    if (item->object_id == O_XIAN_SPEARMAN) {
        swap = Object_Get(O_XIAN_SPEARMAN_STATUE);
    } else {
        swap = Object_Get(O_XIAN_KNIGHT_STATUE);
    }
    return Item_DrawAnimatingEx(
        item,
        &(ITEM_DRAW_DESC) {
            .mesh_mask = item->mesh_bits,
            .mesh_swap = swap,
        });
}
