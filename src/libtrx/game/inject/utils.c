#include "game/inject/utils.h"

#include "game/objects/common.h"

INJECTION_OBJECT_INFO Inject_ReadObjectPtr(VFILE *const fp)
{
    INJECTION_OBJECT_INFO obj_info = {
        .type = VFile_ReadS32(fp),
        .id = Object_FromGameID(VFile_ReadS32(fp)),
    };

    if (obj_info.type == OBJ_TYPE_OBJECT) {
        VFile_Skip(fp, 16);
    }

    return obj_info;
}
