// An item pool with no engine behind it, so the tests can drive the real item
// surface with no level loaded.
//
// Two behaviours are modelled faithfully because the surface's contract rests
// on them: Item_Create/Item_Destroy bump the slot's handle generation, so a
// handle to a recycled slot goes stale; and Item_SetName refuses a duplicate,
// which is what makes assigning a name already in use raise.

#include "fake_engine_items.h"

#include <trx/core/handle.h>
#include <trx/game/anims.h>
#include <trx/game/creature.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/objects/names.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/vars.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/rooms.h>

#include <stdio.h>
#include <string.h>

#define FAKE_OBJ_COUNT 5
#define FAKE_ANIM_COUNT 2
#define FAKE_PROP_SLOTS 4

// The code bits sit in the middle of the flag word.
#define M_CODE_BITS_SHIFT 9

FAKE_ITEM_CALLS g_FakeItemCalls;

static ITEM m_Items[FAKE_ITEM_POOL];
static bool m_Used[FAKE_ITEM_POOL];
static char m_Names[FAKE_ITEM_POOL][32];
static int32_t m_Count;
static uint32_t m_Gens[FAKE_ITEM_POOL];
static HANDLE_REGISTRY m_Handles;

static OBJECT m_Objects[FAKE_OBJ_COUNT];
static ANIM m_Anims[FAKE_ANIM_COUNT];
static ANIM_FRAME m_Frames[64];

// Only max_hit_points is modelled: it is the one property the field table
// itself writes through.
static struct {
    char name[32];
    TRX_VALUE value;
} m_Props[FAKE_ITEM_POOL][FAKE_PROP_SLOTS];

// max_hit_points is an object property, not an ITEM/OBJECT member: that split
// is exactly what item.properties overlays, so the default lives here.
static int32_t m_ObjectHP[FAKE_OBJ_COUNT];

const OBJECT_ID g_CreatureObjects[] = { FAKE_OBJ_WOLF, NO_OBJECT };
const OBJECT_ID g_LoyalObjects[] = { NO_OBJECT };
const OBJECT_ID g_PickupObjects[] = { FAKE_OBJ_VASE, FAKE_OBJ_KEY, NO_OBJECT };
const OBJECT_ID g_NullObjects[] = { NO_OBJECT };
const OBJECT_ID g_AnimObjects[] = { NO_OBJECT };
const OBJECT_ID g_InvObjects[] = { NO_OBJECT };

void FakeItems_Reset(void)
{
    g_FakeItemCalls = (FAKE_ITEM_CALLS) { 0 };
    memset(m_Items, 0, sizeof(m_Items));
    memset(m_Used, 0, sizeof(m_Used));
    memset(m_Names, 0, sizeof(m_Names));
    memset(m_Props, 0, sizeof(m_Props));

    // A fresh level: the generations persist across the reset, so a handle held
    // from before it goes stale, as one held across a real level change does.
    if (m_Handles.gens == nullptr) {
        Handle_RegistryInit(&m_Handles, m_Gens, FAKE_ITEM_POOL);
    }
    Handle_RegistryBumpAll(&m_Handles);

    for (int32_t i = 0; i < FAKE_ANIM_COUNT; i++) {
        m_Anims[i] = (ANIM) {
            .frame_base = 0,
            .frame_end = 10,
            .frame_ptr = m_Frames,
        };
    }

    memset(m_Objects, 0, sizeof(m_Objects));
    m_Objects[FAKE_OBJ_WOLF] = (OBJECT) {
        .loaded = true,
        .intelligent = true,
        .anim_idx = 0,
        .anim_count = FAKE_ANIM_COUNT,
        .mesh_count = 6,
        .radius = 341,
        .shadow_size = 128,
        .smartness = 0x7fff,
        .pivot_length = 375,
    };
    m_Objects[FAKE_OBJ_VASE] = (OBJECT) {
        .loaded = true,
        .anim_idx = 0,
        .anim_count = 1,
        // swap_mesh measures a mesh number against this.
        .mesh_count = 3,
    };
    m_Objects[FAKE_OBJ_UNLOADED] = (OBJECT) { .loaded = false };
    m_Objects[FAKE_OBJ_KEY] = (OBJECT) {
        .loaded = true,
        .anim_idx = 0,
        .anim_count = 1,
        .mesh_count = 1,
    };

    m_ObjectHP[FAKE_OBJ_WOLF] = 20;
    m_ObjectHP[FAKE_OBJ_VASE] = 0;
    m_ObjectHP[FAKE_OBJ_KEY] = 0;

    m_Count = 2;
    for (int32_t i = 0; i < m_Count; i++) {
        m_Used[i] = true;
        m_Items[i] = (ITEM) {
            .object_id = FAKE_OBJ_WOLF,
            .room_num = 0,
            .pos = { .x = 1024 * i, .y = 0, .z = 0 },
            .hit_points = 20,
            .max_hit_points = 20,
            .status = IS_INACTIVE,
        };
    }
    m_Items[1].object_id = FAKE_OBJ_VASE;
    m_Items[1].room_num = 1;
}

// --- items ---

ITEM *Item_Get(const int16_t num)
{
    if (num < 0 || num >= FAKE_ITEM_POOL) {
        return nullptr;
    }
    return &m_Items[num];
}

int16_t Item_GetIndex(const ITEM *const item)
{
    return (int16_t)(item - m_Items);
}

int32_t Item_GetTotalCount(void)
{
    return m_Count;
}

int16_t Item_Create(void)
{
    for (int32_t i = 0; i < FAKE_ITEM_POOL; i++) {
        if (!m_Used[i]) {
            m_Used[i] = true;
            m_Items[i] = (ITEM) {};
            Handle_RegistryBump(&m_Handles, i);
            if (i >= m_Count) {
                m_Count = i + 1;
            }
            return (int16_t)i;
        }
    }
    return NO_ITEM;
}

void Item_Initialise(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    const OBJECT *const obj = Object_Get(item->object_id);
    item->hit_points = m_ObjectHP[item->object_id];
    item->max_hit_points = m_ObjectHP[item->object_id];
    item->status = IS_INACTIVE;
    item->anim_num = obj->anim_idx;
    item->is_collidable = true;
}

void Item_Destroy(const int16_t item_num)
{
    g_FakeItemCalls.kill++;
    ITEM *const item = &m_Items[item_num];
    item->is_destroyed = true;
    item->status = IS_DEACTIVATED;
    item->hit_points = 0;
    item->active = false;
    Handle_RegistryBump(&m_Handles, item_num);
    m_Used[item_num] = false;
    m_Names[item_num][0] = '\0';
}

TRX_HANDLE Item_GetHandle(const int16_t item_num)
{
    return Handle_RegistryMint(&m_Handles, item_num);
}

ITEM *Item_FromHandle(const TRX_HANDLE handle)
{
    if (!Handle_RegistryIsLive(&m_Handles, handle) || handle.id >= m_Count) {
        return nullptr;
    }
    return Item_Get((int16_t)handle.id);
}

void Item_AddSimulated(const int16_t item_num)
{
    m_Items[item_num].active = true;
}

void Item_RemoveSimulated(const int16_t item_num)
{
    m_Items[item_num].active = false;
}

void Item_Activate(const int16_t item_num, const bool force)
{
    ITEM *const item = &m_Items[item_num];
    if (item->active) {
        return;
    }
    item->status = IS_ACTIVE;
    Item_AddSimulated(item_num);
    if (Object_Get(item->object_id)->intelligent) {
        LOT_EnableBaddieAI(item_num, force);
    }
}

void Item_Deactivate(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    Item_RemoveSimulated(item_num);
    if (Object_Get(item->object_id)->intelligent) {
        LOT_DisableBaddieAI(item_num);
    }
    if (item->status == IS_ACTIVE) {
        item->status = IS_INACTIVE;
    }
}

bool Item_IsTriggerActiveRO(const ITEM *const item)
{
    const bool ok = (item->flags & IF_REVERSE) == 0;
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return !ok;
    }
    if (item->timer == 0) {
        return ok;
    }
    if (item->timer == -1) {
        return !ok;
    }
    return ok;
}

int32_t Item_GetTriggerMask(const ITEM *const item)
{
    return (item->flags & IF_CODE_BITS) >> M_CODE_BITS_SHIFT;
}

void Item_SetTriggerMask(ITEM *const item, const int32_t mask)
{
    item->flags &= ~IF_CODE_BITS;
    item->flags |= (mask << M_CODE_BITS_SHIFT) & IF_CODE_BITS;
}

// A stand-in for the real primitive (which test_trigger exercises directly):
// enough of the flag work for a binding test to see the item change, keyed on
// the same kind the bridge builds.
void Item_Trigger(const int16_t item_num, const ITEM_TRIGGER *const trigger)
{
    ITEM *const item = &m_Items[item_num];
    item->timer = (int16_t)trigger->timer;
    switch (trigger->kind) {
    case ITEM_TRIGGER_SWITCH:
    case ITEM_TRIGGER_HEAVY_SWITCH:
        item->flags ^= trigger->mask;
        break;
    case ITEM_TRIGGER_ANTI:
        item->flags &= ~trigger->mask;
        break;
    default:
        item->flags |= trigger->mask;
        break;
    }
    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return;
    }
    if (trigger->one_shot) {
        item->flags |= IF_ONE_SHOT;
    }
    Item_Activate(item_num, false);
}

void Item_UpdateRoom(const int16_t item_num, const int16_t room_num)
{
    if (room_num != NO_ROOM) {
        m_Items[item_num].room_num = room_num;
    }
}

int32_t Item_GetDistance(const ITEM *const item, const XYZ_32 target)
{
    const int32_t dx = target.x - item->pos.x;
    const int32_t dy = target.y - item->pos.y;
    const int32_t dz = target.z - item->pos.z;
    return (int32_t)(dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy)
        + (dz < 0 ? -dz : dz);
}

bool Item_IsAlive(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    return obj->intelligent && item->hit_points > 0 && !item->is_destroyed;
}

bool Item_SetName(const int16_t item_num, const char *const name)
{
    if (name == nullptr) {
        m_Names[item_num][0] = '\0';
        m_Items[item_num].name = nullptr;
        return true;
    }
    for (int32_t i = 0; i < FAKE_ITEM_POOL; i++) {
        if (i != item_num && m_Names[i][0] != '\0'
            && strcmp(m_Names[i], name) == 0) {
            return false;
        }
    }
    snprintf(m_Names[item_num], sizeof(m_Names[item_num]), "%s", name);
    m_Items[item_num].name = m_Names[item_num];
    return true;
}

ITEM *Item_GetByName(const char *const name)
{
    for (int32_t i = 0; i < FAKE_ITEM_POOL; i++) {
        if (m_Used[i] && strcmp(m_Names[i], name) == 0) {
            return &m_Items[i];
        }
    }
    return nullptr;
}

// --- anims ---

ANIM *Item_GetAnim(const ITEM *const item)
{
    return &m_Anims[item->anim_num];
}

int16_t Item_GetRelativeAnim(const ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    return item->anim_num - obj->anim_idx;
}

int16_t Item_GetRelativeFrame(const ITEM *const item)
{
    return item->frame_num - Item_GetAnim(item)->frame_base;
}

ANIM *Anim_GetAnim(const int32_t anim_idx)
{
    return &m_Anims[anim_idx];
}

int32_t Anim_GetTotalCount(void)
{
    return FAKE_ANIM_COUNT;
}

// --- objects ---

OBJECT *Object_Get(const OBJECT_ID object_id)
{
    if (object_id < 0 || object_id >= O_NUMBER_OF) {
        return nullptr;
    }
    // Every id the engine knows resolves to a definition, loaded or not - a
    // lookup that walks the whole catalog reads `loaded` on each. The fake
    // models a handful; the rest read as present but unloaded.
    static OBJECT m_Unloaded = { .loaded = false };
    if (object_id >= FAKE_OBJ_COUNT) {
        return &m_Unloaded;
    }
    return &m_Objects[object_id];
}

// The names an object answers to. The lookup that fuzzy-matches them is Lua
// now, so all the engine has to do is say what they are.
static const char *const m_WolfNames[] = { "wolf", nullptr };
static const char *const m_VaseNames[] = { "vase", "large vase", nullptr };
static const char *const m_KeyNames[] = { "key", nullptr };

const VECTOR *Object_GetNames(const OBJECT_ID obj_id)
{
    // The fake level localizes nothing, so a lookup always takes the
    // compile-time fallback.
    return nullptr;
}

const char *const *Object_GetDefaultNames(const OBJECT_ID obj_id)
{
    if (obj_id == FAKE_OBJ_WOLF) {
        return m_WolfNames;
    }
    if (obj_id == FAKE_OBJ_VASE) {
        return m_VaseNames;
    }
    if (obj_id == FAKE_OBJ_KEY) {
        return m_KeyNames;
    }
    return nullptr;
}

OBJECT *Object_TryGet(const OBJECT_ID object_id)
{
    return Object_Get(object_id);
}

void Object_SwapAllMeshes(const OBJECT_ID obj1_id, const OBJECT_ID obj2_id)
{
    g_FakeItemCalls.swap_mesh++;
}

void Object_SwapMeshEx(
    const OBJECT_ID obj1_id, const OBJECT_ID obj2_id, const int32_t mesh1_num,
    const int32_t mesh2_num)
{
    g_FakeItemCalls.swap_mesh++;
}

// The object's own properties, which every item of the type inherits.
bool ObjectProperty_GetObjectValue(
    const OBJECT *const obj, const char *const name, TRX_VALUE *const out_value)
{
    if (obj == nullptr || strcmp(name, "max_hit_points") != 0) {
        return false;
    }
    *out_value = (TRX_VALUE) {
        .type = TVT_S32,
        .as_int = m_ObjectHP[obj - m_Objects],
    };
    return true;
}

bool ObjectProperty_SetObjectValueRaw(
    OBJECT *const obj, const char *const name, const TRX_VALUE value)
{
    if (obj == nullptr || strcmp(name, "max_hit_points") != 0) {
        return false;
    }
    m_ObjectHP[obj - m_Objects] = value.as_int;
    return true;
}

int32_t ObjectProperty_GetObjectNameCount(const OBJECT *const obj)
{
    return obj != nullptr ? 1 : 0;
}

const char *ObjectProperty_GetObjectName(
    const OBJECT *const obj, const int32_t i)
{
    return i == 0 ? "max_hit_points" : nullptr;
}

bool Object_IsType(const OBJECT_ID object_id, const OBJECT_ID *const test_arr)
{
    for (int32_t i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

// --- object properties ---

bool ObjectProperty_GetItemValue(
    const ITEM *const item, const char *const name, TRX_VALUE *const out_value)
{
    if (item == nullptr) {
        return false;
    }
    const int16_t idx = Item_GetIndex(item);
    for (int32_t i = 0; i < FAKE_PROP_SLOTS; i++) {
        if (strcmp(m_Props[idx][i].name, name) == 0) {
            *out_value = m_Props[idx][i].value;
            return true;
        }
    }
    if (strcmp(name, "max_hit_points") == 0) {
        *out_value = (TRX_VALUE) {
            .type = TVT_S32,
            .as_int = m_ObjectHP[item->object_id],
        };
        return true;
    }
    return false;
}

bool ObjectProperty_SetItemValueRaw(
    ITEM *const item, const char *const name, const TRX_VALUE value)
{
    if (item == nullptr || strcmp(name, "max_hit_points") != 0) {
        return false;
    }
    const int16_t idx = Item_GetIndex(item);
    for (int32_t i = 0; i < FAKE_PROP_SLOTS; i++) {
        if (m_Props[idx][i].name[0] == '\0'
            || strcmp(m_Props[idx][i].name, name) == 0) {
            snprintf(
                m_Props[idx][i].name, sizeof(m_Props[idx][i].name), "%s", name);
            m_Props[idx][i].value = value;
            if (value.type == TVT_S32) {
                item->max_hit_points = value.as_int;
            }
            return true;
        }
    }
    return false;
}

int32_t ObjectProperty_GetItemNameCount(const ITEM *const item)
{
    return item == nullptr ? 0 : 1;
}

const char *ObjectProperty_GetItemName(const ITEM *const item, const int32_t i)
{
    return i == 0 ? "max_hit_points" : nullptr;
}

// --- creatures, pathing, rooms ---

bool Creature_IsHostile(const ITEM *const item)
{
    return Object_Get(item->object_id)->intelligent && Item_IsAlive(item);
}

void Creature_Die(const int16_t item_num, const bool explode)
{
    g_FakeItemCalls.creature_die++;
    g_FakeItemCalls.creature_die_explode = explode;
    m_Items[item_num].hit_points = 0;
}

int32_t Item_Shatter(
    const int16_t item_num, const int32_t mesh_bits, const int16_t damage)
{
    g_FakeItemCalls.shatter++;
    g_FakeItemCalls.shatter_damage = damage;
    return 0;
}

bool LOT_EnableBaddieAI(const int16_t item_num, const bool always)
{
    g_FakeItemCalls.enable_baddie_ai++;
    g_FakeItemCalls.enable_baddie_ai_forced = always;
    return true;
}

void LOT_DisableBaddieAI(const int16_t item_num)
{
    g_FakeItemCalls.disable_baddie_ai++;
}

// A negative x is outside the level; everything else is room 0.
int16_t Room_GetIndexFromPos(const XYZ_32 pos)
{
    return pos.x < 0 ? NO_ROOM : 0;
}
