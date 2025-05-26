#include "game/inject/utils.h"

#include "game/objects/common.h"

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(VFILE *const fp)
{
    INJECTION_OBJECT_INFO obj_info = {
        .type = VFile_ReadS32(fp),
        .id = VFile_ReadS32(fp),
    };

    if (obj_info.type == OBJ_TYPE_OBJECT) {
        UUID uuid;
        VFile_Read(fp, uuid.bytes, sizeof(UUID));
        obj_info.id = Object_UnmapUUID(uuid);
    }

    return obj_info;
}
