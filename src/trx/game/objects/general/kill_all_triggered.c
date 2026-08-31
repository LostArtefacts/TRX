#include <trx/game/effects.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/general/shoal.h>
#include <trx/game/pathing/lot.h>

// The simulated cast this trigger claims, minus the fixtures a mass kill
// leaves standing: Lara, the save crystal, pickups, doors, and a filled
// receptacle (control-less, on the simulation list only as a marker).
static void M_DestroyAllSimulatedItems(void)
{
    int16_t item_num = Item_GetNextSimulated();
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        const int16_t next_item_num = item->next_simulated;

        if (item->is_simulated && !item->trigger.reversed
            && item->object_id != O_LARA
            && item->object_id != O_SAVE_CRYSTAL_ITEM
            && !ObjectFamily_Has(item->object_id, OBJ_FAMILY_PICKUP)
            && !ObjectFamily_Has(item->object_id, OBJ_FAMILY_DOOR)
            && !ObjectFamily_Has(item->object_id, OBJ_FAMILY_RECEPTACLE)) {
            Item_Destroy(item_num);

            if (ObjectFamily_Has(item->object_id, OBJ_FAMILY_SHOAL)) {
                Shoal_TriggerDeactivate(item);
            } else {
                const OBJECT *const obj = Object_Get(item->object_id);
                if (obj->intelligent) {
                    LOT_DisableBaddieAI(item_num);
                    item->hit_points = 0;
                }
            }
        }
        item_num = next_item_num;
    }
}

// Active effects with a control routine, sparing a flame whose counter has
// gone negative.
static void M_DestroyAllActiveEffects(void)
{
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        const int16_t next_effect_num = effect->next_active;
        const OBJECT *const obj = Object_Get(effect->object_id);

        if (obj->effect_control_func != nullptr
            && (effect->object_id != O_FLAME || effect->counter >= 0)) {
            Effect_Destroy(effect_num);
        }
        effect_num = next_effect_num;
    }
}

static void M_Control(const int16_t item_num)
{
    M_DestroyAllSimulatedItems();
    M_DestroyAllActiveEffects();
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_KILL_ALL_TRIGGERED, M_Setup)
