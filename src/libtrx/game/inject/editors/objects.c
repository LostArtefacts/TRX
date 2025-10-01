#include "game/inject.h"
#include "game/objects.h"

static void M_ObjectTypeEdits(
    const INJECTION *const injection, const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        const INJECTION_OBJECT_INFO base_obj_info =
            Inject_ReadObjectPtr(injection->fp);
        const INJECTION_OBJECT_INFO target_obj_info =
            Inject_ReadObjectPtr(injection->fp);

        if (base_obj_info.id < O_FIRST || base_obj_info.id >= O_NUMBER_OF
            || target_obj_info.id < O_FIRST
            || target_obj_info.id >= O_NUMBER_OF) {
            continue;
        }

        OBJECT *const base_obj = Object_Get(base_obj_info.id);
        const OBJECT *const target_obj = Object_Get(target_obj_info.id);
        base_obj->setup_func = target_obj->setup_func;
    }
}

REGISTER_INJECT_EDITOR(IDT_OBJ_TYPE_EDITS, M_ObjectTypeEdits)
