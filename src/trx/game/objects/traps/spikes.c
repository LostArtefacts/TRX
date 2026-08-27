#include <trx/config.h>
#include <trx/game/lara.h>
#include <trx/game/objects/property.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#define M_FALL_SPEED_LIMIT (g_TRVersion == 1 ? 0 : GRAVITY)
#define M_DEFAULT_DAMAGE 15

typedef struct {
    int32_t damage;
} M_PRIV;

static bool M_ShouldImpaleLara(
    const ITEM *const item, const ITEM *const lara_item)
{
    if (lara_item->hit_points > 0) {
        return false;
    }

    int16_t room_num = lara_item->room_num;
    const SECTOR *sector = Room_GetSector(lara_item->pos, &room_num);
    const int32_t height_at_lara = Room_GetHeight(sector, lara_item->pos);

    room_num = item->room_num;
    sector = Room_GetSector(item->pos, &room_num);
    const int32_t height_at_item = Room_GetHeight(sector, item->pos);

    return ABS(height_at_lara - height_at_item) <= WALL_L;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    if (lara_item->hit_points < 0) {
        return;
    }

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!Collide_TestCollision(item, lara_item)) {
        return;
    }

    int32_t blood_spawn_count = Random_GetControl() / 0x6000;
    if (lara_item->gravity) {
        if (lara_item->fall_speed > M_FALL_SPEED_LIMIT
            && !g_Config.debug.enable_invulnerability) {
            Lara_Kill();
            blood_spawn_count = 20;
        }
    } else if (lara_item->speed < 30) {
        return;
    }

    Lara_TakeDamage(p->damage, false);
    for (int32_t i = 0; i < blood_spawn_count; i++) {
        const XYZ_32 pos = {
            .x = lara_item->pos.x + (Random_GetControl() - 0x4000) / 256,
            .z = lara_item->pos.z + (Random_GetControl() - 0x4000) / 256,
            .y = lara_item->pos.y - Random_GetControl() / 64,
        };
        Spawn_Blood(
            pos.x, pos.y, pos.z, 20, Random_GetControl(), item->room_num);
    }

    if (M_ShouldImpaleLara(item, lara_item)) {
        Item_SwitchToAnim(lara_item, LA(LA_SPIKE_DEATH), 0);
        lara_item->current_anim_state = LS(LS_DEATH);
        lara_item->goal_anim_state = LS(LS_DEATH);
        lara_item->pos.y = item->pos.y;
        lara_item->gravity = false;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->save_anim = true;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara hits the spikes without dying instantly."));
}

REGISTER_OBJECT(O_SPIKES, M_Setup)
