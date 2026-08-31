#include <trx/game/objects/links.h>

#include <trx/core/json/util/file.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/objects/names.h>
#include <trx/game/paths.h>

#include <string.h>

typedef struct {
    OBJECT_ID from;
    OBJECT_ID to;
} M_PAIR;

static VECTOR *m_Links[OBJ_LINK_NUMBER_OF] = {};

static const char *const m_Names[OBJ_LINK_NUMBER_OF] = {
#define X_OBJECT_LINK(link_id_, name_) [link_id_] = name_,
#include <trx/game/objects/links.def>
#undef X_OBJECT_LINK
};

static OBJECT_LINK M_FromName(const char *const name)
{
    for (int32_t i = 0; i < OBJ_LINK_NUMBER_OF; i++) {
        if (strcmp(m_Names[i], name) == 0) {
            return (OBJECT_LINK)i;
        }
    }
    return OBJ_LINK_NUMBER_OF;
}

static RESULT M_ReadPair(
    JSON_ARRAY *const pair, const char *const path, const char *const name,
    M_PAIR *const out)
{
    FAIL_IF(
        pair == nullptr || pair->length != 2,
        "%s: every row of '%s' must hold two names", path, name);
    const char *const from = JSON_ArrayGetString(pair, 0, nullptr);
    const char *const to = JSON_ArrayGetString(pair, 1, nullptr);
    out->from = from == nullptr ? NO_OBJECT : Object_IdFromKey(from);
    out->to = to == nullptr ? NO_OBJECT : Object_IdFromKey(to);
    FAIL_IF(
        out->from == NO_OBJECT, "%s: '%s' names no object of this game", path,
        from == nullptr ? "" : from);
    FAIL_IF(
        out->to == NO_OBJECT, "%s: '%s' names no object of this game", path,
        to == nullptr ? "" : to);
    return OK;
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
        const OBJECT_LINK link = M_FromName(name);
        JSON_ARRAY *const rows = JSON_ValueAsArray(elem->value);
        if (link == OBJ_LINK_NUMBER_OF) {
            result = FAIL("%s: there is no link called '%s'", path, name);
            break;
        }
        if (rows == nullptr) {
            result = FAIL("%s: '%s' must hold a list", path, name);
            break;
        }
        if (m_Links[link] == nullptr) {
            m_Links[link] = Vector_Create(sizeof(M_PAIR));
        }
        for (JSON_ARRAY_ELEMENT *row = rows->start; row != nullptr;
             row = row->next) {
            M_PAIR pair;
            result =
                M_ReadPair(JSON_ValueAsArray(row->value), path, name, &pair);
            if (!IS_OK(result)) {
                break;
            }
            Vector_Add(m_Links[link], &pair);
        }
    }

    JSON_ValueFree(root);
    return result;
}

static RESULT M_Load(void)
{
    const char *path = nullptr;
    MUST(GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "object_links.json5", &path));
    return M_LoadFrom(path);
}

static void M_Shutdown(void)
{
    for (int32_t i = 0; i < OBJ_LINK_NUMBER_OF; i++) {
        if (m_Links[i] != nullptr) {
            Vector_Free(m_Links[i]);
            m_Links[i] = nullptr;
        }
    }
}

OBJECT_ID ObjectLink_Get(const OBJECT_ID from, const OBJECT_LINK link)
{
    return ObjectLink_GetAt(from, link, 0);
}

OBJECT_ID ObjectLink_GetInverse(const OBJECT_ID to, const OBJECT_LINK link)
{
    ASSERT(link < OBJ_LINK_NUMBER_OF);
    const VECTOR *const rows = m_Links[link];
    for (int32_t i = 0; rows != nullptr && i < rows->count; i++) {
        const M_PAIR *const pair = Vector_Get(rows, i);
        if (pair->to == to) {
            return pair->from;
        }
    }
    return NO_OBJECT;
}

int32_t ObjectLink_GetCount(const OBJECT_ID from, const OBJECT_LINK link)
{
    ASSERT(link < OBJ_LINK_NUMBER_OF);
    const VECTOR *const rows = m_Links[link];
    int32_t count = 0;
    for (int32_t i = 0; rows != nullptr && i < rows->count; i++) {
        const M_PAIR *const pair = Vector_Get(rows, i);
        count += pair->from == from ? 1 : 0;
    }
    return count;
}

OBJECT_ID ObjectLink_GetAt(
    const OBJECT_ID from, const OBJECT_LINK link, const int32_t idx)
{
    ASSERT(link < OBJ_LINK_NUMBER_OF);
    const VECTOR *const rows = m_Links[link];
    int32_t count = 0;
    for (int32_t i = 0; rows != nullptr && i < rows->count; i++) {
        const M_PAIR *const pair = Vector_Get(rows, i);
        if (pair->from != from) {
            continue;
        }
        if (count == idx) {
            return pair->to;
        }
        count++;
    }
    return NO_OBJECT;
}

int32_t ObjectLink_GetPairCount(const OBJECT_LINK link)
{
    ASSERT(link < OBJ_LINK_NUMBER_OF);
    return m_Links[link] == nullptr ? 0 : m_Links[link]->count;
}

void ObjectLink_GetPairAt(
    const OBJECT_LINK link, const int32_t idx, OBJECT_ID *const from,
    OBJECT_ID *const to)
{
    ASSERT(link < OBJ_LINK_NUMBER_OF);
    ASSERT(idx >= 0 && idx < ObjectLink_GetPairCount(link));
    const M_PAIR *const pair = Vector_Get(m_Links[link], idx);
    *from = pair->from;
    *to = pair->to;
}

REGISTER_BASE_SUBSYSTEM(.load = M_Load, .shutdown = M_Shutdown)
