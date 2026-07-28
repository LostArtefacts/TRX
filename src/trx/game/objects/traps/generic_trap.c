#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_DEFAULT_TOUCH  (-1)
#define M_DEFAULT_DAMAGE 50
#define M_DEFAULT_BLOOD  3
#define M_DEFAULT_PUSH   true
#define M_MAX_BLOOD      10
// clang-format on

typedef enum {
    // clang-format off
    M_STATE_OFF = 0,
    M_STATE_ON  = 1,
    // clang-format on
} M_STATE;

typedef struct {
    int32_t touch_mask;
    int32_t damage;
    int32_t blood_intensity;
    bool push_lara;
} M_PRIV;

// Each of these holds its value to what the trap can use, so a change made
// while the level runs is held to it too.
static void M_SetTouchMask(ITEM *const item, const TRX_VALUE *const in)
{
    M_PRIV *const p = item->priv;
    // A negative mask means every mesh, which is the whole of the field.
    p->touch_mask = in->as_int < 0 ? INT32_MAX : in->as_int;
}

static const char *M_CheckDamage(const TRX_VALUE *const in)
{
    return in->as_int < 0 ? "damage is below nothing" : nullptr;
}

static const char *M_CheckBloodIntensity(const TRX_VALUE *const in)
{
    return in->as_int < 0 || in->as_int > M_MAX_BLOOD
        ? "blood intensity is outside what the trap can spawn"
        : nullptr;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    if (p->push_lara) {
        Object_Collision(item_num, lara_item, coll);
    } else {
        Object_Collision_Trap(item_num, lara_item, coll);
    }
}

static void M_TouchLara(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->damage == 0) {
        return;
    }

    Lara_TakeDamage(p->damage, true);

    if (p->blood_intensity == 0) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const XYZ_32 pos = lara_item->pos;
    Spawn_BloodBath(
        pos.x, pos.y - WALL_L / 2, pos.z, Random_GetDraw() >> 10,
        item->rot.y + DEG_90, lara_item->room_num, p->blood_intensity);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;

    if (Item_IsTriggerActive(item)) {
        item->goal_anim_state = M_STATE_ON;
        if ((item->touch_bits & p->touch_mask) != 0) {
            M_TouchLara(item);
        }
    } else {
        item->goal_anim_state = M_STATE_OFF;
    }

    Item_Animate(item);

    if (Object_Get(item->object_id)->mesh_count > 0) {
        XYZ_32 pos = {};
        Collide_GetJointAbsPosition(item, &pos, 0);
        int16_t room_num = item->room_num;
        Room_GetSector(pos, &room_num);
        Item_UpdateRoom(item_num, room_num);
    }

    if (item->is_finished) {
        Item_RemoveSimulated(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_SETTER(
            M_PRIV, touch_mask, M_DEFAULT_TOUCH, nullptr, M_SetTouchMask,
            "A bitmask of damaging mesh numbers. The default value indicates "
            "all meshes are damaging."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, damage, M_DEFAULT_DAMAGE, M_CheckDamage,
            "Damage dealt when Lara touches the trap. Value range: minimum "
            "0."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, blood_intensity, M_DEFAULT_BLOOD, M_CheckBloodIntensity,
            "The intensity of blood to spawn when Lara is damaged. Value "
            "range: minimum 0; maximum 10."),
        OBJECT_PROPERTY(
            M_PRIV, push_lara, true,
            "Whether or not Lara should be pushed when colliding with the "
            "trap."));
}

REGISTER_OBJECT(O_GENERIC_TRAP_1, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_2, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_3, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_4, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_5, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_6, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_7, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_8, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_9, M_Setup)
REGISTER_OBJECT(O_GENERIC_TRAP_10, M_Setup)
