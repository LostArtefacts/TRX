#include <trx/game/inject/utils.h>

#include <trx/core/file.h>
#include <trx/debug.h>
#include <trx/game/objects/common.h>

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(const INJECTION *const injection)
{
    INJECTION_OBJECT_INFO obj_info = {
        .type = File_ReadS32(injection->fp),
        .id = File_ReadS32(injection->fp),
    };
    obj_info.slot = obj_info.id;

    if (obj_info.type == OBJ_TYPE_OBJECT) {
        obj_info.id = Object_SlotToID(obj_info.id);
        if (injection->version < INJ_VERSION_5) {
            File_Skip(injection->fp, 16);
        }
    }

    return obj_info;
}

RESULT Inject_GetObject(
    const INJECTION_OBJECT_INFO obj_info, OBJECT **const out_obj)
{
    ASSERT(out_obj != nullptr);
    OBJECT *const obj = Object_TryGet(obj_info.id);
    FAIL_IF(obj == nullptr, "level has no object in slot %d", obj_info.slot);
    *out_obj = obj;
    return OK;
}
