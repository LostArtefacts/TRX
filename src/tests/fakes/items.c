// An item pool with no engine behind it, so the tests can drive the real item
// surface with no level loaded.
//
// Two behaviours are modelled faithfully because the surface's contract rests
// on them: Item_Create/Item_Destroy bump the slot's handle generation, so a
// handle to a recycled slot goes stale; and Item_SetName refuses a duplicate,
// which is what makes assigning a name already in use raise.

#include <harness/fake_objects.h>
#include <fakes/items.h>

#include <harness/fake_calls.h>

#include <trx/game/const.h>
#include <trx/core/handle.h>
#include <trx/core/strings.h>
#include <trx/game/anims.h>
#include <trx/game/creature.h>
#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/objects/families.h>
#include <trx/game/objects/names.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/vars.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/rooms.h>

#include <stdio.h>
#include <string.h>

#define FAKE_ANIM_COUNT 2
#define FAKE_FRAME_COUNT 64
#define FAKE_PROP_SLOTS 4

static ITEM m_Items[FAKE_ITEM_POOL];
static bool m_Used[FAKE_ITEM_POOL];
static char m_Names[FAKE_ITEM_POOL][32];
static int32_t m_Count;
static uint32_t m_Gens[FAKE_ITEM_POOL];
static HANDLE_REGISTRY m_Handles;

static OBJECT m_Objects[FAKE_OBJ_COUNT];
static ANIM m_Anims[FAKE_ANIM_COUNT];
static ANIM_FRAME m_Frames[FAKE_FRAME_COUNT];

// Only max_hit_points is modelled: it is the one property the field table
// itself writes through.
static struct {
    char name[32];
    TRX_VALUE value;
} m_Props[FAKE_ITEM_POOL][FAKE_PROP_SLOTS];

// max_hit_points is an object property, not an ITEM/OBJECT member: that split
// is exactly what item.properties overlays, so the default lives here.
static int32_t m_ObjectHP[FAKE_OBJ_COUNT];

// Give each pickup a unique search name. "big urn" shares no words with
// "large medipack".
static const struct {
    OBJECT_ID object_id;
    const char *names;
} m_ObjectNames[] = {
    { FAKE_OBJ_WOLF, "wolf" },
    { FAKE_OBJ_VASE, "vase|large vase|big urn" },
    { FAKE_OBJ_KEY, "key" },
    { FAKE_OBJ_REAL_KEY, "latch" },
    { FAKE_OBJ_PUZZLE, "cog" },
    { FAKE_OBJ_TOOL, "crowbar" },
    { FAKE_OBJ_LEADBAR, "ingot" },
    { FAKE_OBJ_TRINKET, "trinket" },
    // Give both scion states the same name.
    { FAKE_OBJ_SCION, "scion" },
    { FAKE_OBJ_SCION_2, "scion" },
    { FAKE_OBJ_CRYSTAL, "crystal" },
    { NO_OBJECT, nullptr },
};

static int32_t M_FindObjectName(const OBJECT_ID obj_id)
{
    for (int32_t i = 0; m_ObjectNames[i].object_id != NO_OBJECT; i++) {
        if (m_ObjectNames[i].object_id == obj_id) {
            return i;
        }
    }
    return -1;
}

static const char *M_ObjectNames(const OBJECT_ID obj_id)
{
    const int32_t idx = M_FindObjectName(obj_id);
    return idx >= 0 ? m_ObjectNames[idx].names : nullptr;
}

const OBJECT_ID g_CreatureObjects[] = { FAKE_OBJ_WOLF, NO_OBJECT };
const OBJECT_ID g_BossObjects[] = { NO_OBJECT };
const OBJECT_ID g_LoyalObjects[] = { NO_OBJECT };
// The fake's own two, and then the real ones a pickup family names, so a
// command that selects on a family has something here to select. The crystal is
// not among them: it has an inventory icon and nothing else in common with a
// pickup.
const OBJECT_ID g_PickupObjects[] = {
    FAKE_OBJ_VASE,  FAKE_OBJ_KEY,     FAKE_OBJ_REAL_KEY,  FAKE_OBJ_PUZZLE,
    FAKE_OBJ_TOOL,  FAKE_OBJ_LEADBAR, FAKE_OBJ_MEDIPACK,  FAKE_OBJ_TRINKET,
    FAKE_OBJ_SCION, FAKE_OBJ_SCION_2, FAKE_OBJ_WATERSKIN, NO_OBJECT,
};
// Built from pickups.def as the engine builds them: the pickup families are
// read straight off it now, so a test of them sees the real taxonomy. The
// umbrella above stays the fake's own two, which is what the rest of the suite
// counts.
const OBJECT_ID g_GunObjects[] = {
#define X_PICKUP_GUN(item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_GUN
    NO_OBJECT,
};
const OBJECT_ID g_GunAmmoObjects[] = {
#define X_PICKUP_GUN_AMMO(gun_item, item, option) item,
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_GUN_AMMO
    NO_OBJECT,
};
const OBJECT_ID g_SecretObjects[] = {
    O_SECRET_1,
    O_SECRET_2,
    O_SECRET_3,
    NO_OBJECT,
};

const OBJECT_ID g_SwitchObjects[] = { FAKE_OBJ_SWITCH, NO_OBJECT };
const OBJECT_ID g_ReceptacleObjects[] = { FAKE_OBJ_RECEPTACLE, NO_OBJECT };
const OBJECT_ID g_MovableBlockObjects[] = { NO_OBJECT };
const OBJECT_ID g_DoorObjects[] = { FAKE_OBJ_DOOR, NO_OBJECT };
const OBJECT_ID g_NullObjects[] = { NO_OBJECT };
const OBJECT_ID g_AnimObjects[] = { NO_OBJECT };
const OBJECT_ID g_InvObjects[] = { NO_OBJECT };

static void M_Reset(void)
{
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

    // A box that widens frame by frame, so a test can tell the bounds it reads
    // are the current frame's rather than the object's.
    for (int32_t i = 0; i < FAKE_FRAME_COUNT; i++) {
        m_Frames[i].bounds = (BOUNDS_16) {
            .min = { .x = -100 - i, .y = -200, .z = -300 },
            .max = { .x = +100 + i, .y = 0, .z = +300 },
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
    // An object drawn from a sprite keeps it where a modelled one keeps its
    // meshes, and says so by counting them the other way. A pickup is one of
    // those whenever the player turns 3D pickups off.
    m_Objects[FAKE_OBJ_KEY] = (OBJECT) {
        .loaded = true,
        .anim_idx = 0,
        .anim_count = 1,
        .mesh_count = -1,
    };
    m_Objects[FAKE_OBJ_SPRITE] = (OBJECT) {
        .loaded = true,
        .anim_idx = NO_ANIM,
        .mesh_count = -1,
    };

    // The real pickups, loaded so the level can hand them over.
    const OBJECT_ID borrowed[] = {
        FAKE_OBJ_REAL_KEY,  FAKE_OBJ_PUZZLE,  FAKE_OBJ_TOOL,  FAKE_OBJ_LEADBAR,
        FAKE_OBJ_MEDIPACK,  FAKE_OBJ_TRINKET, FAKE_OBJ_SCION, FAKE_OBJ_SCION_2,
        FAKE_OBJ_WATERSKIN, FAKE_OBJ_CRYSTAL,
    };
    for (int32_t i = 0; i < (int32_t)(sizeof(borrowed) / sizeof(borrowed[0]));
         i++) {
        m_Objects[borrowed[i]] = (OBJECT) {
            .loaded = true,
            .anim_idx = 0,
            .anim_count = 1,
            .mesh_count = 1,
        };
    }

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
            .is_visible = true,
            .is_present = true,
        };
    }
    m_Items[1].object_id = FAKE_OBJ_VASE;
    m_Items[1].room_num = 1;
}

// A crystal standing four sectors out, for a test that has to see which pickup
// a command went to: it can be made the nearest one without being the only one.
// Placed on demand, so the level a test reads its counts off is the one above.
void FakeItems_PlaceCrystal(void)
{
    m_Used[m_Count] = true;
    m_Items[m_Count] = (ITEM) {
        .object_id = FAKE_OBJ_CRYSTAL,
        .room_num = 1,
        .pos = { .x = 4 * 1024, .y = 0, .z = 0 },
        .is_visible = true,
        .is_present = true,
    };
    m_Count++;
}

// A scion standing eight sectors out, so a group name can be seen reaching a
// pickup whose collection is its own control routine's business.
void FakeItems_PlaceScion(void)
{
    m_Used[m_Count] = true;
    m_Items[m_Count] = (ITEM) {
        .object_id = FAKE_OBJ_SCION,
        .room_num = 1,
        .pos = { .x = 8 * 1024, .y = 0, .z = 0 },
        .is_visible = true,
        .is_present = true,
    };
    m_Count++;
}

FAKE_ON_RESET(M_Reset)

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
    item->is_visible = true;
    item->is_finished = false;
    item->anim_num = obj->anim_idx;
    item->is_collidable = true;
}

void Item_Destroy(const int16_t item_num)
{
    FAKE_RECORD("destroy");
    ITEM *const item = &m_Items[item_num];
    item->is_destroyed = true;
    item->is_finished = true;
    item->hit_points = 0;
    // is_destroyed is terminal and dominant: the engine detaches the item from
    // its room and stops simulating it before setting it, and asserts as much.
    item->is_simulated = false;
    item->is_present = false;
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
    m_Items[item_num].is_simulated = true;
}

void Item_RemoveSimulated(const int16_t item_num)
{
    m_Items[item_num].is_simulated = false;
}

bool Item_IsInPlay(const ITEM *const item)
{
    return item->is_simulated && item->is_visible && !item->is_finished;
}

bool Item_IsInactive(const ITEM *const item)
{
    return item->is_visible && !item->is_finished && !item->is_simulated;
}

void Item_Activate(const int16_t item_num, const bool force)
{
    ITEM *const item = &m_Items[item_num];
    if (item->is_simulated) {
        return;
    }
    item->is_visible = true;
    item->is_finished = false;
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
    if (Item_IsInPlay(item)) {
        item->is_visible = true;
        item->is_finished = false;
    }
}

bool Item_IsTriggerActiveRO(const ITEM *const item)
{
    const bool ok = !item->trigger.reversed;
    if (item->trigger.mask != TRIGGER_MASK_ALL) {
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
    return item->trigger.mask;
}

void Item_SetTriggerMask(ITEM *const item, const int32_t mask)
{
    item->trigger.mask = mask & TRIGGER_MASK_ALL;
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
        item->trigger.mask ^= trigger->mask;
        break;
    case ITEM_TRIGGER_ANTI:
        item->trigger.mask &= ~trigger->mask;
        break;
    default:
        item->trigger.mask |= trigger->mask;
        break;
    }
    if (item->trigger.mask != TRIGGER_MASK_ALL) {
        return;
    }
    if (trigger->one_shot) {
        item->trigger.spent = true;
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

bool Item_IsTargetable(const ITEM *const item)
{
    return item->hit_points > 0 && Item_IsInPlay(item);
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

const BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *const item)
{
    return &m_Frames[item->frame_num % FAKE_FRAME_COUNT].bounds;
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
    if (object_id < 0 || object_id >= FAKE_OBJ_COUNT) {
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

OBJECT_MESH *Object_GetMesh(const int32_t index)
{
    // The fake level stages no geometry. Callers hand the result straight back
    // to another fake, which records that it was asked rather than reading it.
    static OBJECT_MESH m_Mesh = {};
    return &m_Mesh;
}

const char *Object_GetName(const OBJECT_ID obj_id)
{
    // The fake level localizes nothing, so a lookup always takes the
    // compile-time fallback.
    return Object_GetDefaultName(obj_id);
}

const char *Object_GetAliases(const OBJECT_ID obj_id)
{
    return Object_GetDefaultAliases(obj_id);
}

const char *Object_GetDefaultName(const OBJECT_ID obj_id)
{
    const char *const names = M_ObjectNames(obj_id);
    if (names == nullptr) {
        return nullptr;
    }
    const char *const sep = strchr(names, '|');
    return sep == nullptr
        ? names
        : String_FormatStatic("%.*s", (int32_t)(sep - names), names);
}

const char *Object_GetDefaultAliases(const OBJECT_ID obj_id)
{
    const char *const names = M_ObjectNames(obj_id);
    const char *const sep = names != nullptr ? strchr(names, '|') : nullptr;
    return sep != nullptr ? sep + 1 : nullptr;
}

OBJECT *Object_TryGet(const OBJECT_ID object_id)
{
    return Object_Get(object_id);
}

void Object_SwapAllMeshes(const OBJECT_ID obj1_id, const OBJECT_ID obj2_id)
{
    FAKE_RECORD("swap_mesh");
}

void Object_SwapMeshEx(
    const OBJECT_ID obj1_id, const OBJECT_ID obj2_id, const int32_t mesh1_num,
    const int32_t mesh2_num)
{
    FAKE_RECORD("swap_mesh");
}

void Object_SwapSprite(const OBJECT_ID obj1_id, const OBJECT_ID obj2_id)
{
    FAKE_RECORD("swap_sprite");
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

const char *ObjectProperty_SetObjectValueRaw(
    OBJECT *const obj, const char *const name, const TRX_VALUE value)
{
    if (obj == nullptr || strcmp(name, "max_hit_points") != 0) {
        return "no such property";
    }
    m_ObjectHP[obj - m_Objects] = value.as_int;
    return nullptr;
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

bool ObjectFamily_Has(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    return false;
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

const char *ObjectProperty_SetItemValueRaw(
    ITEM *const item, const char *const name, const TRX_VALUE value)
{
    if (item == nullptr || strcmp(name, "max_hit_points") != 0) {
        return "no such property";
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
            return nullptr;
        }
    }
    return "no room for another property";
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

bool Creature_IsAlly(const ITEM *const item)
{
    return false;
}

void Creature_Die(const int16_t item_num, const bool explode)
{
    FAKE_RECORD("creature_die", FV(explode));
    m_Items[item_num].hit_points = 0;
}

// Records the blow and takes the hit points; the events the real function
// fires are driven directly in the item tests.
void Item_TakeDamage(
    ITEM *const item, const int16_t damage, const ITEM_DAMAGE_FLAGS flags,
    const ITEM *const sender)
{
    FAKE_RECORD("take_damage", FV(damage));
    item->hit_points -= damage;
    if (item->hit_points < 0) {
        item->hit_points = 0;
    }
}

int32_t Item_Shatter(
    const int16_t item_num, const int32_t mesh_bits, const int16_t damage)
{
    FAKE_RECORD("shatter", FV(damage));
    return 0;
}

bool LOT_EnableBaddieAI(const int16_t item_num, const bool always)
{
    FAKE_RECORD("enable_baddie_ai", FV(always));
    return true;
}

void LOT_DisableBaddieAI(const int16_t item_num)
{
    FAKE_RECORD("disable_baddie_ai");
}

// A negative x is outside the level; everything else is room 0.
int16_t Room_GetIndexFromPos(const XYZ_32 pos)
{
    return pos.x < 0 ? NO_ROOM : 0;
}
