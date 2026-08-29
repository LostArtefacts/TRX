#include <trx/game/savegame/identity.h>

#include <trx/core/log.h>

static int32_t m_DroppedCount = 0;

bool SaveGame_WriteIdentity(
    JSON_WRITE_IO *const io, const char *const slot_field,
    const char *const key_field, const CATALOG_CONTEXT context,
    const CATALOG_ID id)
{
    const int32_t slot = Catalog_IDToSlot(context, id, -1);
    const char *const key = Catalog_IDToKey(context, id);
    if (slot < 0 && key == nullptr) {
        return false;
    }
    if (slot >= 0) {
        JSONW_WRITE(io, slot_field, slot);
    }
    if (key != nullptr) {
        JSONW_WRITE(io, key_field, key);
    }
    return true;
}

RESULT SaveGame_ReadIdentity(
    JSON_READ_IO *const io, const char *const slot_field,
    const char *const key_field, const CATALOG_CONTEXT context,
    CATALOG_ID *const out_id)
{
    const char *key = nullptr;
    MUST(JSON_READ_OPT(io, key_field, &key));
    if (key != nullptr) {
        const CATALOG_ID id = Catalog_KeyToID(context, key, NO_CATALOG_ID);
        if (id >= 0) {
            *out_id = id;
            return OK;
        }
    }

    int32_t slot = -1;
    MUST(JSON_READ_OPT(io, slot_field, &slot));
    const CATALOG_ID id = Catalog_SlotToID(context, slot, NO_CATALOG_ID);
    if (id < 0) {
        return JSON_ReadIO_Fail(
            io, "no identity called '%s' or held in slot %d",
            key != nullptr ? key : "", slot);
    }
    *out_id = id;
    return OK;
}

void SaveGame_NoteDropped(const char *const what)
{
    m_DroppedCount++;
    LOG_WARNING("dropping a record this game cannot place: %s", what);
}

int32_t SaveGame_GetDroppedCount(void)
{
    return m_DroppedCount;
}

void SaveGame_ResetDropped(void)
{
    m_DroppedCount = 0;
}
