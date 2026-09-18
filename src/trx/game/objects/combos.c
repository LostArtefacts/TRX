#include <trx/game/objects/combos.h>

#include <trx/core/json/util/file.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/objects/names.h>
#include <trx/game/paths.h>

typedef struct {
    OBJECT_ID source_1;
    OBJECT_ID source_2;
    OBJECT_ID result;
} M_COMBO;

static VECTOR *m_Combos = nullptr;

static RESULT M_ReadObject(
    JSON_ARRAY *const row, const int32_t idx, const char *const path,
    OBJECT_ID *const out)
{
    const char *const name = JSON_ArrayGetString(row, idx, nullptr);
    *out = name == nullptr ? NO_OBJECT : Object_IdFromKey(name);
    FAIL_IF(
        *out == NO_OBJECT, "%s: '%s' names no object of this game", path,
        name == nullptr ? "" : name);
    return OK;
}

static RESULT M_ReadCombo(
    JSON_ARRAY *const row, const char *const path, M_COMBO *const out)
{
    FAIL_IF(
        row == nullptr || row->length != 3,
        "%s: every row must hold three names", path);
    MUST(M_ReadObject(row, 0, path, &out->source_1));
    MUST(M_ReadObject(row, 1, path, &out->source_2));
    MUST(M_ReadObject(row, 2, path, &out->result));
    FAIL_IF(
        out->source_1 == out->source_2,
        "%s: an object cannot combine with itself", path);
    return OK;
}

static RESULT M_LoadFrom(const char *const path)
{
    JSON_VALUE *root = nullptr;
    MUST(JSONFile_ReadRequired(path, &root));
    JSON_ARRAY *const rows = JSON_ValueAsArray(root);
    RESULT result =
        rows == nullptr ? FAIL("%s: the file must hold a list", path) : OK;

    m_Combos = Vector_Create(sizeof(M_COMBO));
    for (JSON_ARRAY_ELEMENT *row = rows == nullptr ? nullptr : rows->start;
         row != nullptr && IS_OK(result); row = row->next) {
        M_COMBO combo;
        result = M_ReadCombo(JSON_ValueAsArray(row->value), path, &combo);
        if (IS_OK(result)) {
            Vector_Add(m_Combos, &combo);
        }
    }

    JSON_ValueFree(root);
    return result;
}

static RESULT M_Load(void)
{
    const char *path = nullptr;
    MUST(GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "object_combos.json5", &path));
    return M_LoadFrom(path);
}

static void M_Shutdown(void)
{
    Vector_Free(m_Combos);
    m_Combos = nullptr;
}

// Return the partner named by a combination row, or NO_OBJECT if the row does
// not name the object.
static OBJECT_ID M_GetPartner(
    const M_COMBO *const combo, const OBJECT_ID object_id)
{
    if (combo->source_1 == object_id) {
        return combo->source_2;
    }
    if (combo->source_2 == object_id) {
        return combo->source_1;
    }
    return NO_OBJECT;
}

int32_t ObjectCombo_GetPartnerCount(const OBJECT_ID object_id)
{
    int32_t count = 0;
    for (int32_t i = 0; m_Combos != nullptr && i < m_Combos->count; i++) {
        const M_COMBO *const combo = Vector_Get(m_Combos, i);
        count += M_GetPartner(combo, object_id) != NO_OBJECT ? 1 : 0;
    }
    return count;
}

OBJECT_ID ObjectCombo_GetPartnerAt(const OBJECT_ID object_id, const int32_t idx)
{
    int32_t count = 0;
    for (int32_t i = 0; m_Combos != nullptr && i < m_Combos->count; i++) {
        const M_COMBO *const combo = Vector_Get(m_Combos, i);
        const OBJECT_ID partner = M_GetPartner(combo, object_id);
        if (partner == NO_OBJECT) {
            continue;
        }
        if (count == idx) {
            return partner;
        }
        count++;
    }
    return NO_OBJECT;
}

OBJECT_ID ObjectCombo_GetResult(
    const OBJECT_ID object_id_1, const OBJECT_ID object_id_2)
{
    for (int32_t i = 0; m_Combos != nullptr && i < m_Combos->count; i++) {
        const M_COMBO *const combo = Vector_Get(m_Combos, i);
        if (M_GetPartner(combo, object_id_1) == object_id_2) {
            return combo->result;
        }
    }
    return NO_OBJECT;
}

REGISTER_BASE_SUBSYSTEM(.load = M_Load, .shutdown = M_Shutdown)
