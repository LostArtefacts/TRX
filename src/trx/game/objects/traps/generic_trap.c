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

static void M_Initialise(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->touch_mask = M_DEFAULT_TOUCH;
    p->damage = M_DEFAULT_DAMAGE;
    p->blood_intensity = M_DEFAULT_BLOOD;
    p->push_lara = M_DEFAULT_PUSH;

    OBJECT_PROPERTY_VALUE value = {};
    if (ObjectProperty_GetItemValue(item, "touch_mask", &value)) {
        p->touch_mask = value.as_int;
    }
    if (ObjectProperty_GetItemValue(item, "damage", &value)) {
        p->damage = value.as_int;
    }
    if (ObjectProperty_GetItemValue(item, "blood_intensity", &value)) {
        p->blood_intensity = value.as_int;
    }
    if (ObjectProperty_GetItemValue(item, "push_lara", &value)) {
        p->push_lara = value.as_bool;
    }

    if (p->touch_mask < 0) {
        p->touch_mask = INT32_MAX;
    }
    CLAMPL(p->damage, 0);
    CLAMP(p->blood_intensity, 0, M_MAX_BLOOD);
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

    if (item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "touch_mask", M_DEFAULT_TOUCH,
            "A bitmask of damaging mesh numbers. The default value indicates "
            "all meshes are damaging."),
        OBJECT_PROPERTY_INT(
            "damage", M_DEFAULT_DAMAGE,
            "Damage dealt when Lara touches the trap."),
        OBJECT_PROPERTY_INT(
            "blood_intensity", M_DEFAULT_BLOOD,
            "The intensity of blood to spawn when Lara is damaged. Value "
            "range: minimum 0; maximum 10."),
        OBJECT_PROPERTY_BOOL(
            "push_lara", true,
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
