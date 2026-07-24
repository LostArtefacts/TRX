// The Great Pyramid shootable Scion.

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/effects.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

typedef struct {
    int32_t counter;
} M_PRIV;

static bool M_ShouldSpawnBlood(const ITEM *const item)
{
    return !g_Config.visuals.fix_texture_issues;
}

static bool M_CanTakeDamage(const ITEM *const item)
{
    return Item_IsInPlay(item);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (Item_IsTriggerActive(item)) {
        if (!LOT_EnableBaddieAI(item_num, true)) {
            return;
        }
        item->is_visible = true;
    }

    M_PRIV *const p = item->priv;
    if (item->hit_points > 0) {
        p->counter = 0;
        Item_Animate(item);
        return;
    }

    if (p->counter == 0) {
        item->is_visible = false;
        item->hit_points = 0;
        Room_TestTriggers(item);
        Item_DetachFromRoom(item_num);
    }

    if (p->counter % 10 == 0) {
        int16_t effect_num = Effect_Create(item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *effect = Effect_Get(effect_num);
            effect->pos.x = item->pos.x + (Random_GetControl() - 0x4000) / 32;
            effect->pos.y =
                item->pos.y + (Random_GetControl() - 0x4000) / 256 - 500;
            effect->pos.z = item->pos.z + (Random_GetControl() - 0x4000) / 32;
            effect->speed = 0;
            effect->frame_num = 0;
            effect->object_id = O_EXPLOSION_1;
            effect->counter = 0;
            Sound_Effect(SFX_EXPLOSION_1, &effect->pos, SPM_NORMAL);
            g_Camera.bounce = -200;
        }
    }

    p->counter++;
    if (p->counter >= LOGIC_FPS * 3) {
        Item_Destroy(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->can_take_damage_func = M_CanTakeDamage;
    obj->should_spawn_blood_func = M_ShouldSpawnBlood;

    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_hitpoints = true;
    OBJECT_PROPERTIES(
        obj, OBJECT_PROPERTY_INT("max_hit_points", 5, "Maximum hit points."));
}

REGISTER_OBJECT(O_SCION_ITEM_3, M_Setup)
