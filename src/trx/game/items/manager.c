#include <trx/game/items/manager.h>

#include <trx/core/handle.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/const.h>
#include <trx/game/game.h>
#include <trx/game/game_buf.h>
#include <trx/game/game_flow.h>
#include <trx/game/items/carrier.h>
#include <trx/game/lara/common.h>
#include <trx/game/lua/events.h>
#include <trx/game/objects.h>
#include <trx/game/output/const.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/rules.h>
#include <trx/game/sparks.h>
#include <trx/version.h>

#include <string.h>

static int32_t m_LevelItemCount = 0;
static int16_t m_MaxUsedItemCount = 0;
static ITEM *m_Items = nullptr;
static int16_t m_NextItemSimulated = NO_ITEM;
static int16_t m_NextItemFree = NO_ITEM;
// One generation per slot, kept apart from the pooled ITEM storage so it
// outlives a level and a handle held across the change goes stale.
static uint32_t m_ItemGens[MAX_ITEMS];
static HANDLE_REGISTRY m_ItemHandles;

static void M_RemoveFromDrawQueues(
    const int16_t item_num, const int16_t room_num)
{
    if (room_num == NO_ROOM) {
        return;
    }
    Room_RemoveDrawnItem(room_num, item_num);
    const ROOM *const room = Room_Get(room_num);
    if (room != nullptr && room->portals != nullptr) {
        for (int32_t i = 0; i < room->portals->count; i++) {
            Room_RemoveDrawnItem(room->portals->portal[i].room_num, item_num);
        }
        M_RemoveFromDrawQueues(item_num, room->flipped_room);
    }
}

static BOUNDS_32 M_GetOccupancyBounds(const ITEM *const item)
{
    const BOUNDS_16 *const b = &Object_Get(item->object_id)->anim_bounds;
    const int32_t radius =
        MAX(MAX(ABS((int32_t)b->min.x), ABS((int32_t)b->max.x)),
            MAX(ABS((int32_t)b->min.z), ABS((int32_t)b->max.z)));
    return (BOUNDS_32) {
        .min = {
            .x = item->pos.x - radius,
            .y = item->pos.y + b->min.y,
            .z = item->pos.z - radius,
        },
        .max = {
            .x = item->pos.x + radius,
            .y = item->pos.y + b->max.y,
            .z = item->pos.z + radius,
        },
    };
}

static void M_AddToDrawQueues(const int16_t item_num, const int16_t room_num)
{
    if (room_num == NO_ROOM) {
        return;
    }
    Room_AddDrawnItem(room_num, item_num);
    const ROOM *const room = Room_Get(room_num);
    if (room == nullptr || room->portals == nullptr) {
        return;
    }
    const BOUNDS_32 bounds = M_GetOccupancyBounds(&m_Items[item_num]);
    for (int32_t i = 0; i < room->portals->count; i++) {
        const PORTAL *const portal = &room->portals->portal[i];
        if (Room_BoundsReachPortal(&bounds, portal)) {
            Room_AddDrawnItem(portal->room_num, item_num);
        }
    }
}

// Splice the item out of the room item chain that collision and
// Item_FindTypeInRoom walk.
static void M_UnlinkChain(const int16_t item_num, const int16_t room_num)
{
    if (room_num == NO_ROOM) {
        return;
    }
    ITEM *const item = &m_Items[item_num];
    ROOM *const room = Room_Get(room_num);
    int16_t link_num = room->item_num;
    if (link_num == item_num) {
        room->item_num = item->next_item;
    } else {
        while (link_num != NO_ITEM) {
            if (m_Items[link_num].next_item == item_num) {
                m_Items[link_num].next_item = item->next_item;
                break;
            }
            link_num = m_Items[link_num].next_item;
        }
    }
}

// A body leaves the active list as it dies, so the fade cannot ride on the
// loop below. The OG counts it off while drawing the room, which leaves a
// corpse nobody is looking at hanging around; this runs either way.
static void M_ControlFades(void)
{
    // Read every frame, so changing the rule mid-fade changes the slope from
    // here on.
    const int32_t speed = g_Rules.corpse.fade_speed;
    for (int16_t item_num = 0; item_num < m_MaxUsedItemCount; item_num++) {
        ITEM *const item = &m_Items[item_num];
        if (item->fade <= 0) {
            continue;
        }
        item->fade -= speed;
        if (item->fade <= 0) {
            Item_Destroy(item_num);
        }
    }
}

void Item_InitialiseItems(const int32_t num_items)
{
    // From here until live play begins, the level's cast and any save overlaid
    // on it are wired up; keep their lifecycle events quiet (see
    // Game_IsSettingUpItems).
    Game_SetIsSettingUpItems(true);

    m_Items = GameBuf_Alloc(sizeof(ITEM) * MAX_ITEMS, GBUF_ITEMS);
    m_LevelItemCount = num_items;
    m_MaxUsedItemCount = num_items;
    m_NextItemFree = num_items;
    m_NextItemSimulated = NO_ITEM;

    for (int32_t i = 0; i < MAX_ITEMS; i++) {
        m_Items[i].properties = (ITEM_PROPERTY_SET) {};
    }

    // A handle retained across a level change must not rebind to what now
    // occupies the same slot index, so the whole pool is retired at once.
    if (m_ItemHandles.gens == nullptr) {
        Handle_RegistryInit(&m_ItemHandles, m_ItemGens, MAX_ITEMS);
    }
    Handle_RegistryBumpAll(&m_ItemHandles);

    for (int32_t i = m_NextItemFree; i < MAX_ITEMS - 1; i++) {
        ITEM *const item = &m_Items[i];
        item->is_simulated = false;
        item->next_item = i + 1;
    }
    m_Items[MAX_ITEMS - 1].next_item = NO_ITEM;
}

void Item_Reset(void)
{
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ObjectProperty_ResetItem(Item_Get(i));
    }
    m_LevelItemCount = 0;
    m_MaxUsedItemCount = 0;
    m_Items = nullptr;
    m_NextItemSimulated = NO_ITEM;
    m_NextItemFree = NO_ITEM;
}

ITEM *Item_Get(const int16_t item_num)
{
    if (item_num == NO_ITEM || m_Items == nullptr || item_num < 0
        || item_num >= MAX_ITEMS) {
        return nullptr;
    }
    return &m_Items[item_num];
}

int16_t Item_GetIndex(const ITEM *const item)
{
    if (item == nullptr || m_Items == nullptr || item < m_Items
        || item >= m_Items + MAX_ITEMS) {
        return NO_ITEM;
    }
    return item - m_Items;
}

TRX_HANDLE Item_GetHandle(const int16_t item_num)
{
    return Handle_RegistryMint(&m_ItemHandles, item_num);
}

ITEM *Item_FromHandle(const TRX_HANDLE handle)
{
    if (!Handle_RegistryIsLive(&m_ItemHandles, handle)
        || handle.id >= m_MaxUsedItemCount) {
        return nullptr;
    }
    return Item_Get((int16_t)handle.id);
}

bool Item_SetName(const int16_t item_num, const char *const name)
{
    ITEM *const item = Item_Get(item_num);
    if (item == nullptr) {
        return false;
    }
    if (name != nullptr) {
        ITEM *const existing = Item_GetByName(name);
        if (existing != nullptr && existing != item) {
            return false;
        }
    }
    if (name != nullptr) {
        item->name = GameBuf_Alloc(strlen(name) + 1, GBUF_ITEMS);
        strcpy(item->name, name);
    } else {
        item->name = nullptr;
    }
    return true;
}

ITEM *Item_GetByName(const char *const name)
{
    if (name == nullptr) {
        return nullptr;
    }
    // search through all items for matching name
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->name != nullptr && strcmp(item->name, name) == 0) {
            return item;
        }
    }
    return nullptr;
}

int32_t Item_GetLevelCount(void)
{
    return m_LevelItemCount;
}

int32_t Item_GetTotalCount(void)
{
    return m_MaxUsedItemCount;
}

int16_t Item_GetNextSimulated(void)
{
    return m_NextItemSimulated;
}

int16_t Item_Create(void)
{
    const int16_t item_num = m_NextItemFree;
    if (item_num != NO_ITEM) {
        m_Items[item_num].init_flags = 0;
        m_Items[item_num].trigger = (ITEM_TRIGGER_STATE) { 0 };
        m_Items[item_num].is_destroyed = false;
        m_Items[item_num].is_finished = false;
        // A recycled slot must not inherit the previous occupant's name.
        m_Items[item_num].name = nullptr;
        Handle_RegistryBump(&m_ItemHandles, item_num);
        ObjectProperty_ResetItem(&m_Items[item_num]);
        m_NextItemFree = m_Items[item_num].next_item;
    }
    m_MaxUsedItemCount = MAX(m_MaxUsedItemCount, item_num + 1);
    return item_num;
}

int16_t Item_CreateLevelItem(void)
{
    const int16_t item_num = Item_Create();
    if (item_num != NO_ITEM) {
        m_LevelItemCount++;
    }
    return item_num;
}

int16_t Item_Spawn(const ITEM *const item, const OBJECT_ID obj_id)
{
    const int16_t spawn_num = Item_Create();
    if (spawn_num != NO_ITEM) {
        ITEM *const spawn = Item_Get(spawn_num);
        spawn->object_id = obj_id;
        spawn->room_num = item->room_num;
        spawn->pos = item->pos;
        spawn->rot = item->rot;
        Item_Initialise(spawn_num);
        spawn->shade.value_1 = SHADE_NEUTRAL;
    }
    return spawn_num;
}

void Item_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Item_SwitchToAnim(item, 0, 0);
    if (item->anim_num != NO_ANIM) {
        item->goal_anim_state = Item_GetAnim(item)->current_anim_state;
        item->current_anim_state = item->goal_anim_state;
    } else {
        item->goal_anim_state = 0;
        item->current_anim_state = 0;
    }
    item->required_anim_state = 0;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    // Both are written by the max_hit_points property, once the item is far
    // enough along to hold it.
    item->hit_points = 0;
    item->max_hit_points = 0;
    item->timer = 0;
    item->mesh_bits = 0xFFFFFFFF;
    item->touch_bits = 0;
    item->ai_bits = 0;
    item->ai_tag = 0;
    item->after_death = 0;
    item->fade = 0;
    item->creature_data = nullptr;
    item->extra_rotations = nullptr;
    item->priv = nullptr;
    item->carried_item = nullptr;

    item->interp.result.pos = item->pos;
    item->interp.result.rot = item->rot;

    item->is_simulated = false;
    item->is_present = false;
    item->is_destroyed = false;
    item->trigger.spent = false;
    item->is_visible = true;
    item->is_finished = false;
    item->gravity = false;
    item->hit_status = false;
    item->is_collidable = true;
    item->looked_at = false;
    item->enable_interpolation = true;
    item->enable_shadow = true;
    item->dynamic_light = false;
    item->include_in_kill_stats = true;

    // The level-format word decodes into the trigger fields and the axes it
    // seeds. Bit IF_INVISIBLE shares its value with IF_ONE_SHOT; in a level
    // seed it always means invisible, so it is read as such here.
    const uint16_t init_flags = item->init_flags;
    item->trigger = (ITEM_TRIGGER_STATE) {
        .mask = (init_flags & IF_CODE_BITS) >> TRIGGER_MASK_SHIFT,
        .reversed = (init_flags & IF_REVERSE) != 0,
    };

    item->clear_body = (init_flags & IF_DESTROYED) != 0;

    if ((init_flags & IF_INVISIBLE) != 0) {
        item->is_visible = false;
    } else if (g_TRVersion >= 2 && obj->intelligent) {
        item->is_visible = false;
    }

    if (item->trigger.mask == TRIGGER_MASK_ALL) {
        item->trigger.mask = 0;
        item->trigger.reversed = true;
        Item_AddSimulated(item_num);
        item->is_visible = true;
    }

    ROOM *const room = Room_Get(item->room_num);
    item->next_item = room->item_num;
    room->item_num = item_num;
    item->is_present = true;

    const SECTOR *const sector =
        Room_GetWorldSector(room, item->pos.x, item->pos.z);
    item->floor = sector->floor.height;

    if (obj->priv_size != 0) {
        if (item->priv != nullptr) {
            memset(item->priv, 0, obj->priv_size);
        } else {
            item->priv = GameBuf_Alloc(obj->priv_size, GBUF_ITEM_DATA);
        }
    }

    // Before the object's own initialiser, so it reads what it declared.
    ObjectProperty_ApplyToItem(item);

    // TODO: remove GF check once demo config reset is run before level load
    if (Game_IsBonusFlagSet(GBF_NGPLUS)
        && GF_GetCurrentLevel()->type != GFL_DEMO) {
        item->hit_points *= 2;
    }

    if (obj->initialise_func != nullptr) {
        obj->initialise_func(item_num);
    }

    if (item->room_num != NO_ROOM) {
        Room_AddDrawnItem(item->room_num, item_num);
    }

    // The gate keeps the level's starting cast quiet: only an item initialised
    // during live play - a runtime spawn - counts as entering the world.
    if (!Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_ENTER_WORLD, item_num);
    }
}

void Item_StartFade(ITEM *const item)
{
    if (g_Rules.corpse.fade_speed <= 0 || item->fade > 0) {
        return;
    }
    item->fade = 255;
}

bool Item_IsFading(const ITEM *const item)
{
    return item->fade > 0;
}

void Item_Control(void)
{
    int16_t item_num = Item_GetNextSimulated();
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        const int16_t next = item->next_simulated;
        const OBJECT *obj = Object_Get(item->object_id);
        if (!item->is_destroyed && obj->control_func != nullptr) {
            obj->control_func(item_num);
        }
        item_num = next;
    }

    M_ControlFades();
    Carrier_AnimateDrops();
}

void Item_Destroy(const int16_t item_num)
{
    Sparks_DetachItem(item_num);
    Item_RemoveSimulated(item_num);
    Item_DetachFromRoom(item_num);

    ITEM *const item = &m_Items[item_num];
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item == lara->target) {
        lara->target = nullptr;
    }

    item->is_destroyed = true;

    // The removals above are what makes is_destroyed terminal; the struct's
    // axis invariants lean on it, so pin it at the sole writer.
    ASSERT(!item->is_simulated && !item->is_present);

    // Fired while the item still resolves: the handler runs synchronously, so
    // it can read the item, but only until the handle below goes stale.
    // on_leave_sim and on_leave_world have already fired for it.
    if (!Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_DESTROY, item_num);
    }
    // Invalidate any script handle to this item, whether or not the slot is
    // recycled below.
    Handle_RegistryBump(&m_ItemHandles, item_num);

    if (item_num >= m_LevelItemCount) {
        item->next_item = m_NextItemFree;
        m_NextItemFree = item_num;
    }

    while (m_MaxUsedItemCount > 0
           && m_Items[m_MaxUsedItemCount - 1].is_destroyed) {
        m_MaxUsedItemCount--;
    }
}

void Item_RemoveSimulated(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    if (!item->is_simulated) {
        return;
    }

    item->is_simulated = false;

    int16_t link_num = m_NextItemSimulated;
    if (link_num == item_num) {
        m_NextItemSimulated = item->next_simulated;
    } else {
        while (link_num != NO_ITEM) {
            if (m_Items[link_num].next_simulated == item_num) {
                m_Items[link_num].next_simulated = item->next_simulated;
                break;
            }
            link_num = m_Items[link_num].next_simulated;
        }
    }

    if (!Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_LEAVE_SIM, item_num);
    }
}

void Item_DetachFromRoom(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    const bool was_present = item->is_present;
    item->is_present = false;

    M_RemoveFromDrawQueues(item_num, item->room_num);
    M_UnlinkChain(item_num, item->room_num);

    // Only when it was in the world, so an already-detached item does not fire.
    if (was_present && !Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_LEAVE_WORLD, item_num);
    }
}

void Item_AddSimulated(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    // A control-less item cannot simulate; leaving is_simulated false is the
    // whole of the resting state this used to force here.
    if (Object_Get(item->object_id)->control_func == nullptr) {
        return;
    }

    if (item->is_simulated) {
        return;
    }

    item->is_simulated = true;
    item->next_simulated = m_NextItemSimulated;
    m_NextItemSimulated = item_num;

    if (!Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_ENTER_SIM, item_num);
    }
}

void Item_Activate(const int16_t item_num, const bool force)
{
    ITEM *const item = &m_Items[item_num];
    const OBJECT *const obj = Object_Get(item->object_id);

    if (item->is_simulated) {
        return;
    }

    if (obj->activate_func != nullptr) {
        obj->activate_func(item);
    } else if (obj->intelligent) {
        if (item->is_visible && !item->is_finished) { // sleeping visible item
            item->touch_bits = 0;
            Item_AddSimulated(item_num);
            LOT_EnableBaddieAI(item_num, true);
        } else if (!item->is_visible) { // hidden ambush item
            item->touch_bits = 0;
            if (LOT_EnableBaddieAI(item_num, force)) {
                Item_SetVisible(item, true);
            }
            Item_AddSimulated(item_num);
        }
    } else {
        item->touch_bits = 0;
        Item_SetVisible(item, true);
        Item_SetFinished(item, false);
        Item_AddSimulated(item_num);
    }

    // The front door ran on an item that was not already simulated.
    // on_enter_sim has fired for whichever of the branches above simulated it;
    // this is the operation on top of that, the one a trigger drives.
    if (!Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_ACTIVATE, item_num);
    }
}

void Item_Deactivate(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    const OBJECT *const obj = Object_Get(item->object_id);
    const bool was_simulated = item->is_simulated;

    // RemoveActive clears is_simulated, which is the whole of the downgrade:
    // is_visible and is_finished are untouched, so a spent item stays spent and
    // an ambushing creature stays hidden, while a running item merely stops.
    Item_RemoveSimulated(item_num);
    if (obj->intelligent) {
        LOT_DisableBaddieAI(item_num);
    }

    // Only when it was running, so an antitrigger on an idle item is not
    // reported as a stop. on_leave_sim has already fired for it.
    if (was_simulated && !Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_DEACTIVATE, item_num);
    }
}

void Item_Respawn(const int16_t item_num, const int16_t room_num)
{
    ITEM *const item = Item_Get(item_num);
    // A recycled slot may still be simulated - a creature whose AI slot was
    // reclaimed keeps is_simulated with no creature data. Item_Activate no-ops
    // on an already-simulated item, so drop it first to force a full wake.
    if (item->is_simulated) {
        Item_RemoveSimulated(item_num);
    }
    // The previous occupant died in this slot, leaving it visible and finished.
    // Clear finished so Item_Activate takes the visible item back into play
    // rather than reading it as a spent corpse. Any fade it had left goes with
    // it, or the new occupant is drawn part-transparent and destroyed once the
    // count reaches zero.
    Item_SetFinished(item, false);
    item->fade = 0;
    // Force the slot back into the room's item chain: a same-room update alone
    // will not relink a slot Item_Destroy left detached, so bounce it through
    // NO_ROOM. Item_Activate then enables the AI with the room settled.
    Item_UpdateRoom(item_num, NO_ROOM);
    Item_UpdateRoom(item_num, room_num);
    Item_Activate(item_num, true);
}

void Item_SetVisible(ITEM *const item, const bool value)
{
    if (item->is_visible == value) {
        return;
    }
    item->is_visible = value;
    if (!Game_IsSettingUpItems()) {
        LUA_FireEventInt32(
            value ? LUA_EVENT_SHOW : LUA_EVENT_HIDE, Item_GetIndex(item));
    }
}

void Item_SetFinished(ITEM *const item, const bool value)
{
    if (item->is_finished == value) {
        return;
    }

    item->is_finished = value;
    if (value && !Game_IsSettingUpItems()) {
        LUA_FireEventInt32(LUA_EVENT_FINISH, Item_GetIndex(item));
    }
}

void Item_UpdateRoom(const int16_t item_num, const int16_t room_num)
{
    ITEM *const item = &m_Items[item_num];
    const int16_t old_room_num = item->room_num;

    M_RemoveFromDrawQueues(item_num, old_room_num);
    M_AddToDrawQueues(item_num, room_num);

    if (old_room_num != room_num) {
        M_UnlinkChain(item_num, old_room_num);

        if (room_num == NO_ROOM) {
            Item_DetachFromRoom(item_num);
            item->room_num = NO_ROOM;
        } else {
            ROOM *const room = Room_Get(room_num);
            item->room_num = room_num;
            item->next_item = room->item_num;
            room->item_num = item_num;
        }

        // After the room lists above settle, so a handler reads a consistent
        // world.
        if (!Game_IsSettingUpItems()) {
            const LUA_EVENT_ARG args[] = {
                { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = item_num } },
                { .type = LUA_EVENT_ARG_INT32,
                  .value = { .i32 = old_room_num } },
                { .type = LUA_EVENT_ARG_INT32, .value = { .i32 = room_num } },
            };
            LUA_FireEventEx(LUA_EVENT_ROOM_CHANGE, args, 3);
        }
    }
}

void Item_InitialiseDrawQueues(void)
{
    for (int32_t i = 0; i < m_LevelItemCount; i++) {
        M_AddToDrawQueues(i, m_Items[i].room_num);
    }
}

int32_t Item_GlobalReplace(
    const OBJECT_ID src_obj_id, const OBJECT_ID dst_obj_id)
{
    int32_t changed = 0;

    for (int32_t item_num = 0; item_num < m_MaxUsedItemCount; item_num++) {
        ITEM *const item = &m_Items[item_num];
        if (item->object_id == src_obj_id) {
            item->object_id = dst_obj_id;
            changed++;
        }
    }

    return changed;
}
