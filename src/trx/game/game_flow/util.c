#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/game.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/savegame.h>

static void M_DisableObject(const OBJECT_ID object_id)
{
    OBJECT *const obj = Object_Get(object_id);
    obj->loaded = false;
    obj->collision_func = nullptr;
    obj->control_func = nullptr;
    obj->draw_func = nullptr;
    obj->floor_height_func = nullptr;
    obj->ceiling_height_func = nullptr;
    obj->block_func = nullptr;
}

static void M_ReplaceObject(
    const OBJECT_ID src_object_id, const OBJECT_ID dst_object_id)
{
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id == src_object_id) {
            item->object_id = dst_object_id;
        }
    }
}

void GF_DisableObjectsIfNeeded(void)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    if (level == nullptr
        || (level->type != GFL_NORMAL && level->type != GFL_BONUS
            && level->type != GFL_GYM)) {
        return;
    }
    if (g_Config.gameplay.disable_medpacks) {
        M_DisableObject(O_SMALL_MEDIPACK_ITEM);
        M_DisableObject(O_LARGE_MEDIPACK_ITEM);
    }

    if (g_Config.gameplay.disable_extra_guns) {
        const RESUME_INFO *const resume = SG_Resume_GetEntry(level);
        ASSERT(resume != nullptr);
        OBJECT_FAMILY_FOR_EACH(OBJ_FAMILY_GUN, gun_id)
        {
            if (Inv_State_Has(&resume->inv, O_PISTOLS_ITEM)) {
                M_DisableObject(gun_id);
            } else {
                M_ReplaceObject(gun_id, O_PISTOLS_ITEM);
            }
            M_DisableObject(Object_GetCognate(gun_id, g_GunAmmoObjectMap));
        }
    }
}
