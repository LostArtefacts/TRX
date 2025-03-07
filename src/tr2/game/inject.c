#include <libtrx/config.h>
#include <libtrx/game/inject.h>

bool Inject_IsRelevant(const INJECTION *const injection)
{
    switch (injection->type) {
    case IFT_GENERAL:
        return true;
    case IFT_FLOOR_DATA:
        return g_Config.gameplay.fix_floor_data_issues;
    case IFT_ITEM_POSITION:
        return g_Config.visuals.fix_item_rots;
    default:
        return false;
    }
}
