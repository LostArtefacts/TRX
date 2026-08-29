#include <trx/game/inject/utils.h>

#include <trx/core/file.h>
#include <trx/game/objects/common.h>

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(const INJECTION *const injection)
{
    INJECTION_OBJECT_INFO obj_info = {
        .type = File_ReadS32(injection->fp),
        .id = File_ReadS32(injection->fp),
    };

    if (obj_info.type == OBJ_TYPE_OBJECT) {
        obj_info.id = Object_SlotToID(obj_info.id);
        if (injection->version < INJ_VERSION_5) {
            File_Skip(injection->fp, 16);
        }
    }

    return obj_info;
}
