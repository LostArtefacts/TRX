#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/items/col.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

#define M_DEFAULT_DAMAGE 25

typedef struct {
    XYZ_32 origin;
    XYZ_32 target;
    int16_t direction;
    int16_t velocity;
    int16_t reverse_timer;
} M_PRIV;

static int32_t M_GetDamage(const ITEM *const item)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DEFAULT_DAMAGE;
}

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    JSON_SHOULD(JSON_READ(io, "origin", &p->origin));
    JSON_SHOULD(JSON_READ(io, "target", &p->target));
    JSON_SHOULD(JSON_READ(io, "direction", &p->direction));
    JSON_SHOULD(JSON_READ(io, "velocity", &p->velocity));
    JSON_SHOULD(JSON_READ(io, "reverse_timer", &p->reverse_timer));
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "origin", p->origin);
    JSONW_WRITE(io, "target", p->target);
    JSONW_WRITE(io, "direction", p->direction);
    JSONW_WRITE(io, "velocity", p->velocity);
    JSONW_WRITE(io, "reverse_timer", p->reverse_timer);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->origin = item->pos;
    p->target = XYZ_32_OffsetYaw(item->pos, item->rot.y, 2560);
    p->direction = 1;
    p->velocity = 0;
    p->reverse_timer = 0;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        return;
    }

    item->current_anim_state = 0;
    Output_AddDynamicLightRGB(
        (XYZ_32) { item->pos.x, item->pos.y - 64, item->pos.z },
        (Random_GetControl() & 1) + 8,
        (RGB_888) {
            (Random_GetControl() & 0x1F) + 192,
            Random_GetControl() & 0x1F,
            Random_GetControl() & 7,
        });
    item->mesh_bits = -1 - (Random_GetControl() & 0x14);
    const int32_t dx = ABS(p->target.x - item->pos.x);
    const int32_t dz = ABS(p->target.z - item->pos.z);

    if (dx < 768 && dz < 768) {
        p->reverse_timer = 32;
        p->target =
            XYZ_32_OffsetYaw(p->origin, item->rot.y, -2560 * p->direction);
    }

    if (p->direction == 1) {
        if (p->reverse_timer != 0) {
            if (p->velocity != 0) {
                if (p->velocity > 4) {
                    p->velocity -= p->velocity >> 2;
                } else {
                    p->velocity = 0;
                }
            } else {
                p->reverse_timer--;
                if (p->reverse_timer == 1) {
                    p->direction = -1;
                }
            }
        } else {
            p->velocity += 5;
            CLAMPG(p->velocity, 512);
        }
    } else if (p->reverse_timer != 0) {
        if (p->velocity != 0) {
            if (p->velocity < -4) {
                p->velocity -= p->velocity >> 2;
            } else {
                p->velocity = 0;
            }
        } else {
            p->reverse_timer--;
            if (p->reverse_timer == 1) {
                p->direction = -p->direction;
            }
        }
    } else {
        p->velocity -= 5;
        CLAMPL(p->velocity, -512);
    }

    item->pos.x += (p->velocity * Math_Sin(item->rot.y)) >> (W2V_SHIFT + 2);
    item->pos.z += (p->velocity * Math_Cos(item->rot.y)) >> (W2V_SHIFT + 2);
    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);

    if (room_num != item->room_num) {
        Item_UpdateRoom(item_num, room_num);
    }

    ITEM *const lara_item = Lara_GetItem();
    if (Item_TestBoundsCollide(item, lara_item, 64)) {
        Lara_TakeDamage(M_GetDamage(item), false);
        Spawn_BloodBathD(
            lara_item->pos.x, item->pos.y - (Random_GetControl() & 0xFF) - 32,
            lara_item->pos.z, (Random_GetControl() & 0x7F) + 128,
            (int16_t)(Random_GetControl() << 1), lara_item->room_num, 3);
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;

    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "damage", M_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the rotating laser."));
}

REGISTER_OBJECT(O_ROTATING_LASER, M_Setup)
