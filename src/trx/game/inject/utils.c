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
    } else if (obj_info.type == OBJ_TYPE_SYMBOL) {
        obj_info.id =
            Inject_ResolveSymbol(injection, CATALOG_OBJECTS, obj_info.slot);
        if (obj_info.id == NO_CATALOG_ID) {
            LOG_WARNING("Slot %d names no object symbol", obj_info.slot);
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

CATALOG_ID Inject_ResolveSymbol(
    const INJECTION *const injection, const CATALOG_CONTEXT context,
    const int32_t slot)
{
    for (int32_t i = 0; i < injection->num_symbols; i++) {
        const INJECTION_SYMBOL *const symbol = &injection->symbols[i];
        if (symbol->context == context && symbol->slot == slot) {
            return symbol->id;
        }
    }
    return NO_CATALOG_ID;
}
