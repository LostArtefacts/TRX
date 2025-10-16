#include "game/inject/utils.h"

#include "game/objects/common.h"

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(const INJECTION *const injection)
{
    INJECTION_OBJECT_INFO obj_info = {
        .type = VFile_ReadS32(injection->fp),
        .id = Object_FromGameID(VFile_ReadS32(injection->fp)),
    };

    if (obj_info.type == OBJ_TYPE_OBJECT
        && injection->version < INJ_VERSION_5) {
        VFile_Skip(injection->fp, 16);
    }

    return obj_info;
}
