#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/fx/debris.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_DAMAGE            150
#define M_SWING_BITS        0b0111'1110'0000
#define M_LOOP_BITS         0b0000'1110'0000
#define M_DEADLY_FRAMES     52
#define M_IMPACT_FRAME      8
#define M_PAUSE             60
#define M_SMASH_DEBRIS      128
// clang-format on

typedef enum {
    M_OCB_LOOP = 0,
    M_OCB_OBELISK = 2,
    M_OCB_ONE_SWING = 3,
    M_OCB_PAUSE = 4,
} M_OCB;

typedef enum {
    // clang-format off
    M_STATE_REST  = 1,
    M_STATE_SWING = 2,
    // clang-format on
} M_STATE;

typedef enum {
    // clang-format off
    M_ANIM_SWING = 1,
    // clang-format on
} M_ANIM;

typedef struct {
    int32_t deadly_bits;
    int16_t pause;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "deadly_bits", &p->deadly_bits));
    MUST(JSON_READ_OPT(io, "pause", &p->pause));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "deadly_bits", p->deadly_bits);
    JSONW_WRITE(io, "pause", p->pause);
}

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static bool M_IsSameSquare(const ITEM *const item1, const ITEM *const item2)
{
    return item1->pos.x == item2->pos.x && item1->pos.z == item2->pos.z;
}

static void M_ShatterBlock(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Sound_Effect(SFX_HIT_ROCK, &item->pos, SPM_NORMAL);

    SPHERE spheres[1];
    if (Collide_GetSpheres(item, spheres, true) > 0) {
        const OBJECT *const obj = Object_Get(item->object_id);
        const SHATTER_ITEM shatter_item = {
            .pos = spheres[0].pos,
            .mesh = Object_GetMesh(obj->mesh_idx),
            .bit = 1,
            .yaw = item->rot.y,
        };
        FX_Debris_ShatterItem(&shatter_item, M_SMASH_DEBRIS, item->room_num, 0);
    }
    item->mesh_bits &= ~1;

    MovableBlock_UpdateBox(item, false);
    Item_Destroy(item_num);
}

static bool M_IsHiddenInBlock(const ITEM *const item)
{
    return item->object_id == O_PUZZLE_ITEM_4_COMBO_1
        || item->object_id == O_PUZZLE_ITEM_4_COMBO_2
        || item->object_id == O_PUZZLE_ITEM_5;
}

static void M_SmashBlocks(const ITEM *const item)
{
    const ROOM *const room = Room_Get(item->room_num);
    bool smashed = false;
    int16_t target_num = room->item_num;
    while (target_num != NO_ITEM) {
        const ITEM *const target = Item_Get(target_num);
        const int16_t next_num = target->next_item;
        if (ObjectFamily_Has(target->object_id, OBJ_FAMILY_HAMMER_SMASHABLE)
            && M_IsSameSquare(target, item)) {
            M_ShatterBlock(target_num);
            smashed = true;
        }
        target_num = next_num;
    }

    if (!smashed) {
        return;
    }

    target_num = room->item_num;
    while (target_num != NO_ITEM) {
        ITEM *const target = Item_Get(target_num);
        if (M_IsHiddenInBlock(target) && M_IsSameSquare(target, item)) {
            Item_SetVisible(target, true);
        }
        target_num = target->next_item;
    }
}

static void M_Strike(const ITEM *const item)
{
    // TODO: start the obelisks of the room once they line up, when the
    // obelisk is ported.
    Sound_Effect(SFX_DOOR_THUD, &item->pos, SPM_NORMAL);
    Sound_Effect(SFX_EXPLOSION_2, &item->pos, SPM_NORMAL);
}

static void M_Swing(ITEM *const item, const int32_t ocb)
{
    M_PRIV *const p = item->priv;
    if (item->current_anim_state == M_STATE_REST
        && item->goal_anim_state == M_STATE_REST) {
        if (p->pause == 0) {
            Item_SwitchToAnim(item, M_ANIM_SWING, 0);
            item->current_anim_state = M_STATE_SWING;
            item->goal_anim_state = M_STATE_SWING;
            p->pause = M_PAUSE;
        } else if (ocb == M_OCB_ONE_SWING) {
            item->trigger.mask = 0;
            p->pause = 0;
        } else if (ocb == M_OCB_PAUSE) {
            p->pause--;
        } else {
            p->pause = 0;
        }
        return;
    }

    item->goal_anim_state = M_STATE_REST;
    const int16_t frame = Item_GetRelativeFrame(item);
    p->deadly_bits = frame < M_DEADLY_FRAMES ? M_SWING_BITS : 0;
    if (frame == M_IMPACT_FRAME) {
        if (ocb == M_OCB_OBELISK) {
            M_Strike(item);
        } else {
            M_SmashBlocks(item);
        }
    } else if (frame > M_DEADLY_FRAMES && ocb == M_OCB_OBELISK) {
        item->trigger.mask = 0;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    Object_Collision_SphereTrap(
        item_num, lara_item, coll, M_DAMAGE, p->deadly_bits, false);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        p->deadly_bits = 0;
        return;
    }

    const int32_t ocb = M_GetOCB(item);
    if (ocb == M_OCB_LOOP) {
        const int16_t frame = Item_GetRelativeFrame(item);
        p->deadly_bits = frame < M_DEADLY_FRAMES ? M_LOOP_BITS : 0;
    } else {
        M_Swing(item, ocb);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_anim = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_HAMMER, M_Setup)
