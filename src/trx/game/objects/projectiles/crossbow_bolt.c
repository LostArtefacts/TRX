#include <trx/game/objects/projectiles/crossbow_bolt.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/game/collision/common.h>
#include <trx/game/fx/debris.h>
#include <trx/game/fx/water.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/registry.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/projectiles/blast.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_SPEED                 512
#define M_MIN_SPEED_UW          64
#define M_HIT_RADIUS            128
#define M_SPLASH_COUNT          8
// How many faces of a shattered static fly off as debris. A negative count
// picks the faces at random.
#define M_STATIC_DEBRIS_FACES   (-128)
#define M_BOLT_DEBRIS_FACES     (-64)
// Makes debris take the shade of the bolt rather than of its room.
#define M_DEBRIS_OWN_SHADE      0x400
// clang-format on

typedef enum {
    M_KIND_NORMAL,
    M_KIND_POISON,
    M_KIND_EXPLOSIVE,
} M_KIND;

typedef struct {
    M_KIND kind;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    int32_t kind = M_KIND_NORMAL;
    MUST(JSON_READ_OPT(io, "kind", &kind));
    p->kind = (M_KIND)kind;
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "kind", (int32_t)p->kind);
}

static void M_Initialise(const int16_t item_num)
{
    M_PRIV *const p = Item_Get(item_num)->priv;
    p->kind = M_KIND_NORMAL;
}

// Breaks the bolt into pieces where it stops, and removes it.
static void M_Break(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    SPHERE spheres[1];
    if (Collide_GetSpheres(item, spheres, true) > 0) {
        const OBJECT *const obj = Object_Get(item->object_id);
        const SHATTER_ITEM shatter_item = {
            .pos = spheres[0].pos,
            .mesh = Object_GetMesh(obj->mesh_idx),
            .bit = 1,
            .yaw = item->rot.y,
            .flags = M_DEBRIS_OWN_SHADE,
        };
        FX_Debris_ShatterItem(
            &shatter_item, M_BOLT_DEBRIS_FACES, item->room_num, 0);
    }
    Item_Destroy(item_num);
}

static void M_HitItems(
    const ITEM *const item, const GAME_VECTOR old_pos,
    const COLLIDED_OBJECTS *const collided)
{
    const M_PRIV *const p = item->priv;
    const GAME_VECTOR hit_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };
    for (int32_t i = 0; i < collided->item_count; i++) {
        ITEM *const target = collided->items[i];
        // TODO: TR4 bolts also trip the shootable switches and break the
        // skeletons, which TRX lacks.
        if (!Object_Get(target->object_id)->intelligent
            || !Item_CanBeProjectileTarget(target)) {
            continue;
        }
        Gun_HitTarget(
            target, &old_pos, &hit_pos, Gun_Registry_Get(LGT_CROSSBOW)->damage);
        if (p->kind == M_KIND_POISON && Item_CanTakeDamage(target)) {
            target->is_poisoned = true;
        }
    }
}

static void M_HitStatics(
    const ITEM *const item, const COLLIDED_OBJECTS *const collided)
{
    for (int32_t i = 0; i < collided->static_count; i++) {
        const STATIC_MESH *const mesh = collided->statics[i];
        if (Object_Get3DStatic(mesh->static_num)->shatter.by_shot) {
            FX_Debris_ShatterStatic(
                mesh, M_STATIC_DEBRIS_FACES, item->room_num, 0);
            Room_ShatterStatic(mesh);
        }
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const GAME_VECTOR old_pos = {
        .pos = item->pos,
        .room_num = item->room_num,
    };

    item->timer++;
    const bool was_underwater = Room_Get(item->room_num)->flags.underwater;
    if (was_underwater) {
        if (item->speed > M_MIN_SPEED_UW) {
            item->speed -= item->speed >> 4;
        }
        if ((item->timer & 1) != 0) {
            Spawn_BubbleEx(&item->pos, item->room_num, 4, 8);
        }
    }

    const int32_t speed_xz = (item->speed * Math_Cos(item->rot.x)) >> W2V_SHIFT;
    item->pos.x += (speed_xz * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.y += (item->speed * Math_Sin(-item->rot.x)) >> W2V_SHIFT;
    item->pos.z += (speed_xz * Math_Cos(item->rot.y)) >> W2V_SHIFT;

    const M_PRIV *const p = item->priv;
    bool exploded = false;
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    if (Room_GetHeight(sector, item->pos) < item->pos.y
        || Room_GetCeiling(sector, item->pos) > item->pos.y) {
        item->pos = old_pos.pos;
        if (p->kind != M_KIND_EXPLOSIVE) {
            M_Break(item_num);
            return;
        }
        exploded = true;
    }

    Item_UpdateRoom(item_num, room_num);
    const ROOM *const room = Room_Get(room_num);
    if (room->flags.underwater && !was_underwater) {
        const XYZ_32 surface = {
            .x = item->pos.x,
            .y = room->max_ceiling,
            .z = item->pos.z,
        };
        Sparks_TriggerSmallSplash(surface, M_SPLASH_COUNT);
        FX_Water_SetupRipple(surface, (Random_GetControl() & 7) + 8, 0);
    }

    // An explosive bolt that touches anything explodes, and looks again
    // with the reach of its blast.
    bool collided = false;
    int32_t radius = exploded ? BLAST_RADIUS : M_HIT_RADIUS;
    for (int32_t i = 0; i < 2; i++) {
        COLLIDED_OBJECTS found;
        Collide_GetCollidedObjects(item, radius, true, true, &found);
        if (found.item_count == 0 && found.static_count == 0) {
            break;
        }
        collided = true;
        if (p->kind != M_KIND_EXPLOSIVE || exploded) {
            if (exploded) {
                Blast_HitObjects(item, &found, false);
            } else {
                M_HitItems(item, old_pos, &found);
                M_HitStatics(item, &found);
            }
            break;
        }
        exploded = true;
        radius = BLAST_RADIUS;
    }

    if (exploded) {
        Blast_Explode(item, old_pos.pos);
    }
    if (collided || exploded) {
        M_Break(item_num);
    }
}

static M_KIND M_GetLoadedKind(void)
{
    switch (Inv_GetAmmoKind(LGT_CROSSBOW)) {
    case O_CROSSBOW_AMMO_2_ITEM:
        return M_KIND_POISON;
    case O_CROSSBOW_AMMO_3_ITEM:
        return M_KIND_EXPLOSIVE;
    default:
        return M_KIND_NORMAL;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
}

int16_t CrossbowBolt_Fire(
    const XYZ_32 pos, const int16_t room_num, const XYZ_16 rot)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return NO_ITEM;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_CROSSBOW_BOLT;
    item->room_num = room_num;
    item->pos = pos;
    item->interp.prev.pos = pos;
    Item_Initialise(item_num);

    item->rot = (XYZ_16) { .x = rot.x, .y = rot.y };
    item->speed = M_SPEED;
    // A negative shade lights the bolt by the room around it as it flies, as
    // the original lights its projectiles.
    item->shade.value_1 = -1;
    item->shade.value_2 = -1;
    M_PRIV *const p = item->priv;
    p->kind = M_GetLoadedKind();
    Item_AddSimulated(item_num);
    return item_num;
}

REGISTER_OBJECT(O_CROSSBOW_BOLT, M_Setup)
