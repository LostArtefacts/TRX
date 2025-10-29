#include "config.h"
#include "debug.h"
#include "game/effects.h"
#include "game/objects.h"
#include "game/rooms.h"
#include "game/savegame/bson.h"

typedef struct {
    int16_t count;
    int16_t id_map[MAX_EFFECTS];
} M_FX_ORDER;

static bool M_LoadXYZ(
    JSON_OBJECT *const obj, const char *const key, XYZ_32 *const target)
{
    ASSERT(target != nullptr);
    const JSON_OBJECT *const sub_obj = JSON_ObjectGetObject(obj, key);
    if (sub_obj == nullptr) {
        return false;
    }
    target->x = JSON_ObjectGetInt(sub_obj, "x", target->x);
    target->y = JSON_ObjectGetInt(sub_obj, "y", target->y);
    target->z = JSON_ObjectGetInt(sub_obj, "z", target->z);
    return true;
}

static bool M_LoadPos(JSON_OBJECT *const obj, XYZ_32 *const target)
{
    ASSERT(target != nullptr);
    if (JSON_ObjectContainsKey(obj, "x")) {
        // TR1X ..v4.15
        target->x = JSON_ObjectGetInt(obj, "x", target->x);
        target->y = JSON_ObjectGetInt(obj, "y", target->y);
        target->z = JSON_ObjectGetInt(obj, "z", target->z);
        return true;
    } else {
        return M_LoadXYZ(obj, "pos", target);
    }
}

static bool M_LoadRot(JSON_OBJECT *const obj, XYZ_16 *const target)
{
    ASSERT(target != nullptr);
    if (JSON_ObjectContainsKey(obj, "x_rot")) {
        // TR1X ..v4.15
        target->x = JSON_ObjectGetInt(obj, "x_rot", target->x);
        target->y = JSON_ObjectGetInt(obj, "y_rot", target->y);
        target->z = JSON_ObjectGetInt(obj, "z_rot", target->z);
        return true;
    } else {
        XYZ_32 target32 = {};
        if (!M_LoadXYZ(obj, "rot", &target32)) {
            return false;
        }
        target->x = target32.x;
        target->y = target32.y;
        target->z = target32.z;
        return true;
    }
}

bool Savegame_BSON_LoadEffects(JSON_ARRAY *const fx_arr)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return true;
    }

    if (fx_arr == nullptr) {
#if 0
        // TR1X ..v2.15.3, TR2X ..v1.1
        LOG_ERROR("Malformed save: invalid or missing effect array");
#endif
        return true;
    }

    if ((signed)fx_arr->length >= MAX_EFFECTS) {
        LOG_WARNING(
            "Malformed save: expected a max of %d effect, got %d. effect over "
            "the maximum will not be created.",
            MAX_EFFECTS - 1, fx_arr->length);
    }

    for (size_t i = 0; i < fx_arr->length; i++) {
        JSON_OBJECT *const fx_obj = JSON_ArrayGetObject(fx_arr, i);
        if (fx_obj == nullptr) {
            LOG_ERROR("Malformed save: invalid effect data");
            return false;
        }

        const int16_t room_num =
            JSON_ObjectGetInt(fx_obj, "room_number", NO_ROOM);
        if (room_num == NO_ROOM) {
            LOG_ERROR("Malformed save: invalid room number in effect %d", i);
            return false;
        }
        const int16_t effect_num = Effect_Create(room_num);
        if (effect_num == NO_EFFECT) {
            continue;
        }
        EFFECT *const effect = Effect_Get(effect_num);
        if (!M_LoadPos(fx_obj, &effect->pos)) {
            LOG_ERROR("Malformed save: missing position data in effect %d", i);
            return false;
        }
        if (!M_LoadRot(fx_obj, &effect->rot)) {
            LOG_ERROR("Malformed save: missing rotation data in effect %d", i);
            return false;
        }
        effect->object_id =
            Object_FromGameID(JSON_ObjectGetInt(fx_obj, "object_number", -1));
        effect->speed = JSON_ObjectGetInt(fx_obj, "speed", 0);
        effect->fall_speed = JSON_ObjectGetInt(fx_obj, "fall_speed", 0);
        effect->frame_num = JSON_ObjectGetInt(fx_obj, "frame_number", 0);
        effect->counter = JSON_ObjectGetInt(fx_obj, "counter", 0);
        effect->shade = JSON_ObjectGetInt(fx_obj, "shade", 0);
    }

    return true;
}
