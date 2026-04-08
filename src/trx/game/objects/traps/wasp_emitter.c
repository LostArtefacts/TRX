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
    int16_t slots[M_MAX_SLOTS];
} M_PRIV;

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    JSON_OPTIONAL(JSON_READ(io, "cooldown", &p->cooldown));
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
        p->slots[i] = NO_ITEM;
    }
}

static void M_PopulateSlots(M_PRIV *const p)
{
    int32_t count = 0;
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id != O_WASP_MUTANT
            || (item->ai_bits & AI_MODIFY) == 0) {
            continue;
        }

        p->slots[count++] = i;
        if (count >= M_MAX_SLOTS) {
            break;
        }
    }
}

static int32_t M_GetEmptySlot(const M_PRIV *const p)
{
    for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
        const ITEM *const item = Item_Get(p->slots[i]);
        if (item->creature_data == nullptr) {
            return i;
        }
    }
    return -1;
}

static int32_t M_GetActiveCount(const M_PRIV *const p)
{
    int32_t count = 0;
    for (int32_t i = 0; i < M_MAX_SLOTS; i++) {
        const ITEM *const item = Item_Get(p->slots[i]);
        if (item->active) {
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

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!item->active || item->timer <= 0) {
        return;
    }

    M_PRIV *const p = item->priv;
    if (p->slots[0] == NO_ITEM) {
        M_PopulateSlots(p);
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
    item->timer -= LOGIC_FPS;

    const int32_t active_count = M_GetActiveCount(item->priv);
    if (active_count >= M_MAX_ACTIVE) {
        return;
    }
    
    M_SpawnWasp(item, m_EmptySlot);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = nullptr;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_flags = true;
}

REGISTER_OBJECT(O_WASP_MUTANT_EMITTER, M_Setup)
