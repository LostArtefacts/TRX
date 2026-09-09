#include <trx/game/inject/utils.h>

#include <trx/core/file.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
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
    } else if (obj_info.type == OBJ_TYPE_SYMBOL) {
        const int32_t symbol_idx = obj_info.id;
        obj_info.id = NO_CATALOG_ID;
        if (symbol_idx < 0 || symbol_idx >= injection->num_symbols) {
            LOG_WARNING("Symbol %d is out of table range", symbol_idx);
        } else if (injection->symbols[symbol_idx].context != CATALOG_OBJECTS) {
            LOG_WARNING("Symbol %d names no object", symbol_idx);
        } else {
            obj_info.id = injection->symbols[symbol_idx].id;
        }
    }

    return obj_info;
}

RESULT Inject_GetObject(
    const INJECTION_OBJECT_INFO obj_info, OBJECT **const out_obj)
{
    ASSERT(out_obj != nullptr);
    OBJECT *const obj = Object_TryGet(obj_info.id);
    if (obj == nullptr) {
        if (obj_info.type == OBJ_TYPE_SYMBOL) {
            return FAIL("the file has no object at symbol %d", obj_info.slot);
        }
        return FAIL("level has no object in slot %d", obj_info.slot);
    }
    *out_obj = obj;
    return OK;
}
