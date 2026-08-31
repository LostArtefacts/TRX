#include <trx/game/objects/families.h>

#include <trx/core/json/util/file.h>
#include <trx/core/memory.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/objects/names.h>
#include <trx/game/paths.h>

#include <string.h>

// Store each object's families in a per-object list, using null for none;
// family lists are short and remain session-local because they share the mod
// lifetime.
static VECTOR **m_Members = nullptr;
static int32_t m_MemberCount = 0;

// Allocate entries through the specified object position so a newly minted
// object can record its families.
static void M_EnsureRoom(const OBJECT_ID object_id)
{
    if (object_id < m_MemberCount) {
        return;
    }
    const int32_t count = object_id + 1;
    m_Members = Memory_Realloc(m_Members, sizeof(VECTOR *) * count);
    memset(
        &m_Members[m_MemberCount], 0,
        sizeof(VECTOR *) * (count - m_MemberCount));
    m_MemberCount = count;
}

static RESULT M_LoadFrom(const char *const path)
{
    JSON_VALUE *root = nullptr;
    MUST(JSONFile_ReadRequired(path, &root));
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    RESULT result = root_obj == nullptr
        ? FAIL("%s: the file must hold a dictionary", path)
        : OK;

    for (JSON_OBJECT_ELEMENT *elem = root_obj == nullptr ? nullptr
                                                         : root_obj->start;
         elem != nullptr && IS_OK(result); elem = elem->next) {
        const char *const name = elem->name->string;
        const CATALOG_ID family =
            Catalog_KeyToID(CATALOG_FAMILIES, name, NO_CATALOG_ID);
        JSON_ARRAY *const members = JSON_ValueAsArray(elem->value);
        if (family == NO_CATALOG_ID) {
            result = FAIL("%s: there is no family called '%s'", path, name);
            break;
        }
        if (members == nullptr) {
            result = FAIL("%s: '%s' must hold a list", path, name);
            break;
        }
        for (JSON_ARRAY_ELEMENT *member = members->start; member != nullptr;
             member = member->next) {
            const char *const key = JSON_ValueGetString(member->value, nullptr);
            const OBJECT_ID object_id =
                key == nullptr ? NO_OBJECT : Object_IdFromKey(key);
            if (object_id == NO_OBJECT) {
                result = FAIL(
                    "%s: '%s' names no object of this game", name,
                    key == nullptr ? "" : key);
                break;
            }
            ObjectFamily_Add(object_id, family);
        }
    }

    JSON_ValueFree(root);
    return result;
}

static RESULT M_Load(void)
{
    const char *path = nullptr;
    MUST(GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "object_families.json5", &path));
    return M_LoadFrom(path);
}

static void M_Shutdown(void)
{
    for (int32_t i = 0; i < m_MemberCount; i++) {
        if (m_Members[i] != nullptr) {
            Vector_Free(m_Members[i]);
        }
    }
    Memory_FreePointer(&m_Members);
    m_MemberCount = 0;
}

bool ObjectFamily_Has(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    ASSERT(Catalog_IsValidID(CATALOG_FAMILIES, family));
    if (object_id < 0 || object_id >= m_MemberCount
        || m_Members[object_id] == nullptr) {
        return false;
    }
    return Vector_Contains(m_Members[object_id], &family);
}

void ObjectFamily_Add(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    ASSERT(Catalog_IsValidID(CATALOG_FAMILIES, family));
    ASSERT(Catalog_IsValidID(CATALOG_OBJECTS, object_id));
    if (ObjectFamily_Has(object_id, family)) {
        return;
    }
    M_EnsureRoom(object_id);
    if (m_Members[object_id] == nullptr) {
        m_Members[object_id] = Vector_Create(sizeof(OBJECT_FAMILY));
    }
    Vector_Add(m_Members[object_id], &family);
}

OBJECT_ID ObjectFamily_GetMember(const OBJECT_FAMILY family, const int32_t idx)
{
    int32_t count = 0;
    OBJECT_FAMILY_FOR_EACH(family, i)
    {
        if (count == idx) {
            return i;
        }
        count++;
    }
    return NO_OBJECT;
}

int32_t ObjectFamily_GetIndex(
    const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    int32_t count = 0;
    OBJECT_FAMILY_FOR_EACH(family, i)
    {
        if (i == object_id) {
            return count;
        }
        count++;
    }
    return -1;
}

void ObjectFamily_Remove(const OBJECT_ID object_id, const OBJECT_FAMILY family)
{
    ASSERT(Catalog_IsValidID(CATALOG_FAMILIES, family));
    if (object_id >= 0 && object_id < m_MemberCount
        && m_Members[object_id] != nullptr) {
        Vector_Remove(m_Members[object_id], &family);
    }
}

REGISTER_BASE_SUBSYSTEM(.load = M_Load, .shutdown = M_Shutdown)
