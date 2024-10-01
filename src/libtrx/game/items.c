#include "game/items.h"

#include "game/const.h"
#include "utils.h"

void Item_TakeDamage(
    ITEM *const item, const int16_t damage, const bool hit_status)
{
#if TR_VERSION == 1
    if (item->hit_points == DONT_TARGET) {
        return;
    }
#endif

    item->hit_points -= damage;
    CLAMPL(item->hit_points, 0);

    if (hit_status) {
        item->hit_status = 1;
    }
}
