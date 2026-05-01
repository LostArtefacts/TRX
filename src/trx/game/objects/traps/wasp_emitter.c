#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>

// clang-format off
#define M_MAX_SLOTS  3
#define M_MAX_ACTIVE 2
#define M_MAX_DIST   SQUARE(WALL_L * 12) // = 150994944
#define M_COOLDOWN   255
// clang-format on

typedef struct {
    int32_t cooldown;
    int32_t spawn_count;
    int32_t spawn_total;
    int16_t slots[M_MAX_SLOTS];
} M_PRIV;

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    JSON_OPTIONAL(JSON_READ(io, "cooldown", &p->cooldown));
    JSON_OPTIONAL(JSON_READ(io, "spawn_count", &p->spawn_count));
    JSON_OPTIONAL(JSON_READ(io, "spawn_total", &p->spawn_total));
    if (JSON_SHOULD(JSON_PUSH(io, "slots"))) {
        for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
            JSON_SHOULD(JSON_READ_A(io, i, &p->slots[i]));
        }
        JSON_POP(io);
    }
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "cooldown", p->cooldown);
    JSONW_WRITE(io, "spawn_count", p->spawn_count);
    JSONW_WRITE(io, "spawn_total", p->spawn_total);
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
        JSONW_PUSH_VALUE(io, p->slots[i]);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "slots");
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
        const int16_t wasp_item_num = Item_CreateLevelItem();
        p->slots[i] = wasp_item_num;
        if (wasp_item_num == NO_ITEM) {
            continue;
        }

        ITEM *const wasp_item = Item_Get(wasp_item_num);
        wasp_item->object_id = O_WASP_MUTANT;
        wasp_item->room_num = item->room_num;
        wasp_item->pos = item->pos;
        wasp_item->rot = item->rot;
        wasp_item->status = IS_INVISIBLE;
        Item_Initialise(wasp_item_num);
    }
}

static const ITEM *M_GetWaspItem(const M_PRIV *const p, const int32_t slot_idx)
{
    const int16_t item_num = p->slots[slot_idx];
    return item_num == NO_ITEM ? nullptr : Item_Get(item_num);
}

static int32_t M_GetEmptySlot(const M_PRIV *const p)
{
    for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
        const ITEM *const item = M_GetWaspItem(p, i);
        if (item != nullptr && item->creature_data == nullptr) {
            return i;
        }
    }
    return -1;
}

static int32_t M_GetActiveCount(const M_PRIV *const p)
{
    int32_t count = 0;
    for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
        const ITEM *const item = M_GetWaspItem(p, i);
        if (item != nullptr && item->active) {
            count++;
        }
    }
    return count;
}

static void M_SpawnWasp(const ITEM *const spawner_item, const int32_t slot_idx)
{
    M_PRIV *const p = spawner_item->priv;

    ITEM *const wasp_item = Item_Get(p->slots[slot_idx]);
    wasp_item->pos = spawner_item->pos;
    wasp_item->rot = spawner_item->rot;
    Item_SwitchToAnim(wasp_item, 0, 0);
    wasp_item->current_anim_state = Item_GetAnim(wasp_item)->current_anim_state;
    wasp_item->goal_anim_state = wasp_item->current_anim_state;
    wasp_item->required_anim_state = 0;
    wasp_item->flags &= ~(IF_INVISIBLE | IF_KILLED | 3);
    wasp_item->creature_data = nullptr;
    wasp_item->hit_points = Object_Get(wasp_item->object_id)->hit_points;
    wasp_item->mesh_bits = -1;
    wasp_item->status = IS_ACTIVE;
    wasp_item->collidable = true;
    wasp_item->ai_bits = AI_MODIFY;

    if (wasp_item->active) {
        Item_RemoveActive(p->slots[slot_idx]);
    }

    Item_AddActive(p->slots[slot_idx]);
    Item_UpdateRoom(p->slots[slot_idx], NO_ITEM);
    Item_UpdateRoom(p->slots[slot_idx], spawner_item->room_num);
    LOT_EnableBaddieAI(p->slots[slot_idx], true);
}

static bool M_Trigger(ITEM *const item, const TRIGGER *const trigger)
{
    if (trigger == nullptr || trigger->type == TT_ANTITRIGGER
        || trigger->type == TT_ANTIPAD) {
        return true;
    }

    item->timer = 0;
    item->flags |= IF_ONE_SHOT;

    M_PRIV *const p = item->priv;
    p->spawn_total = trigger->timer;
    p->spawn_count = 0;
    return true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (!item->active || p->spawn_count >= p->spawn_total) {
        return;
    }

    const int16_t m_EmptySlot = M_GetEmptySlot(p);
    if (m_EmptySlot == -1) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t dist = XYZ_32_GetLength2((XYZ_32) { dx, 0, dz });
    if (ABS(dx) > 32000 || ABS(dz) > 32000 || dist > M_MAX_DIST) {
        return;
    }

    if (p->cooldown > 0) {
        p->cooldown--;
        return;
    }

    p->cooldown = M_COOLDOWN;

    const int32_t active_count = M_GetActiveCount(item->priv);
    if (active_count >= M_MAX_ACTIVE) {
        return;
    }

    p->spawn_count++;
    M_SpawnWasp(item, m_EmptySlot);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->trigger_func = M_Trigger;
    obj->control_func = M_Control;
    obj->draw_func = nullptr;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_flags = true;
}

REGISTER_OBJECT(O_WASP_MUTANT_EMITTER, M_Setup)
