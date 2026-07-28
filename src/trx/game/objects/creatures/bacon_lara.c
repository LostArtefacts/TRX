#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/log.h>
#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>

#define M_SMASH_JUMP_FRAME 1
#define M_MAX_DEATH_COUNT 2
#define M_FALL_RATE 50

typedef struct {
    bool status;
    bool anchored;
    int32_t anchor_room;
    int32_t anchor_x;
    int32_t anchor_z;
    int32_t death_count;
} M_PRIV;

static void M_InitialiseAnchor(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    p->anchored = false;

    // The room she is placed in, unless the level names another.
    const int32_t room_num =
        p->anchor_room >= 0 ? p->anchor_room : item->room_num;
    if (room_num >= Room_GetCount()) {
        LOG_ERROR("Could not anchor Bacon Lara to room %d", room_num);
        return;
    }

    const ROOM *const room = Room_Get(room_num);
    p->anchor_x = room->pos.x + room->size.x * (WALL_L >> 1);
    p->anchor_z = room->pos.z + room->size.z * (WALL_L >> 1);
    p->anchored = true;
}

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    JSON_SHOULD(JSON_READ(io, "status", &p->status));
    JSON_SHOULD(JSON_READ(io, "death_count", &p->death_count));
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "status", p->status);
    JSONW_WRITE(io, "death_count", p->death_count);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    const OBJECT *const lara_obj = Object_Get(O_LARA);
    OBJECT *const bacon_obj = Object_Get(O_BACON_LARA);
    bacon_obj->anim_idx = lara_obj->anim_idx;
    bacon_obj->frame_base = lara_obj->frame_base;
    p->status = false;
    M_InitialiseAnchor(item);
}

static void M_SyncToLara(ITEM *const item, const ITEM *const lara_item)
{
    M_PRIV *const p = item->priv;
    const XYZ_32 pos = {
        .x = 2 * p->anchor_x - lara_item->pos.x,
        .z = 2 * p->anchor_z - lara_item->pos.z,
        .y = lara_item->pos.y,
    };

    int16_t room_num = item->room_num;
    const SECTOR *sector = Room_GetSector(pos, &room_num);
    const int32_t floor_height = Room_GetHeight(sector, pos);
    item->floor = floor_height;

    room_num = lara_item->room_num;
    sector = Room_GetSector(lara_item->pos, &room_num);
    const int32_t lara_floor_height = Room_GetHeight(sector, lara_item->pos);

    const int16_t relative_anim = Item_GetRelativeAnim(lara_item);
    const int16_t relative_frame = Item_GetRelativeFrame(lara_item);
    Item_SwitchToObjAnim(item, relative_anim, relative_frame, O_LARA);
    item->pos = pos;
    item->rot = lara_item->rot;
    item->rot.y -= DEG_180;
    item->fall_speed = lara_item->fall_speed;
    Item_UpdateRoom(Item_GetIndex(item), lara_item->room_num);

    if (floor_height < lara_floor_height + WALL_L || lara_item->gravity) {
        p->death_count = 0;
        return;
    }

    // Bacon Lara runs one frame behind Lara, so the death check must pass twice
    // in succession. This prevents premature death when Lara is, for example,
    // pulling out of water.
    p->death_count++;
    if (p->death_count < M_MAX_DEATH_COUNT) {
        return;
    }

    item->current_anim_state = LS(LS_FAST_FALL);
    item->goal_anim_state = LS(LS_FAST_FALL);
    Item_SwitchToAnim(item, LA(LA_SMASH_JUMP), M_SMASH_JUMP_FRAME);
    item->speed = 0;
    item->fall_speed = 0;
    item->gravity = true;
    item->pos.y += M_FALL_RATE;
    p->status = true;
}

static void M_FallToDeath(ITEM *const item)
{
    Item_Animate(item);

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    const int32_t height = Room_GetHeight(sector, item->pos);
    item->floor = height;

    Room_TestTriggers(item);
    if (item->pos.y >= height) {
        item->floor = height;
        item->pos.y = height;
        Room_TestTriggers(item);
        item->gravity = false;
        item->fall_speed = 0;
        item->goal_anim_state = LS(LS_DEATH);
        item->required_anim_state = LS(LS_DEATH);
        if (room_num != item->room_num) {
            Item_UpdateRoom(Item_GetIndex(item), room_num);
        }
        Item_SetFinished(item, true);
        Item_StartFade(item);
        // The pit is what kills her; damage only ever passes through her to
        // Lara. The tally is untouched, as it was before she reported at all.
        Item_TakeDamage(
            item, item->hit_points, IDF_NO_HIT_STATUS | IDF_NO_KILL_STATS,
            nullptr);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (!p->anchored) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();

    if (Item_IsTriggerActive(item)) {
        if (!LOT_EnableBaddieAI(item_num, true)) {
            return;
        }
        Item_SetVisible(item, true);
    }

    // Her own hit points stand in for Lara's, so they are only worth reading
    // back while she is alive. Once the pit has taken them, they stay taken.
    if (item->hit_points > 0 && item->hit_points < LARA_MAX_HITPOINTS) {
        Lara_TakeDamage((LARA_MAX_HITPOINTS - item->hit_points) * 10, false);
        item->hit_points = LARA_MAX_HITPOINTS;
    }

    if (!p->status) {
        M_SyncToLara(item, lara_item);
    }

    // Synchronizing with Lara may have invoked Bacon Lara's death, hence check
    // the flag again on the same frame.
    if (p->status) {
        M_FallToDeath(item);
    }
}

static bool M_Draw(const ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (p->status || !Item_IsInPlay(item)) {
        return Object_DrawAnimatingItem(item);
    }

    OBJECT_MESH *old_mesh_ptrs[LM_NUMBER_OF];

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        old_mesh_ptrs[mesh] = Lara_Mesh_Get(mesh);
        Lara_Mesh_SwapSingle(mesh, O_BACON_LARA);
    }

    Lara_Draw(item);

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        Lara_Mesh_Set(mesh, old_mesh_ptrs[mesh]);
    }
    return true;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;
    obj->collision_func = Creature_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->shadow_size = (UNIT_SHADOW * 10) / 16;
    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_STORED(
            "max_hit_points", LARA_MAX_HITPOINTS, "Maximum hit points."),
        OBJECT_PROPERTY(
            M_PRIV, anchor_room, -1,
            "Room whose center Bacon Lara mirrors Lara's movement about. "
            "-1 uses the room she is placed in. Value range: minimum -1."));
}

REGISTER_OBJECT(O_BACON_LARA, M_Setup)
