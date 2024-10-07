#include "game/console/history.h"

#include "config/file.h"
#include "memory.h"
#include "utils.h"
#include "vector.h"

#define MAX_HISTORY_ENTRIES 30

VECTOR *m_History = NULL;
static const char *m_Path = "cfg/" PROJECT_NAME "_console_history.json5";

void M_LoadFromJSON(JSON_OBJECT *const root_obj)
{
    JSON_ARRAY *const arr = JSON_ObjectGetArray(root_obj, "entries");
    if (arr == NULL) {
        return;
    }

    Console_History_Clear();
    for (size_t i = 0; i < arr->length; i++) {
        const char *const line = JSON_ArrayGetString(arr, i, NULL);
        if (line != NULL) {
            Console_History_Append(line);
        }
    }
}

void M_DumpToJSON(JSON_OBJECT *const root_obj)
{
    JSON_ARRAY *const arr = JSON_ArrayNew();

    bool has_elements = false;
    for (int32_t i = 0; i < Console_History_GetLength(); i++) {
        JSON_ArrayAppendString(arr, Console_History_Get(i));
        has_elements = true;
    }

    if (has_elements) {
        JSON_ObjectAppendArray(root_obj, "entries", arr);
    } else {
        JSON_ArrayFree(arr);
    }
}

void Console_History_Init(void)
{
    m_History = Vector_Create(sizeof(char *));
    ConfigFile_Read(m_Path, &M_LoadFromJSON);
}

void Console_History_Shutdown(void)
{
    if (m_History != NULL) {
        ConfigFile_Write(m_Path, &M_DumpToJSON);
        for (int32_t i = m_History->count - 1; i >= 0; i--) {
            char *const prompt = *(char **)Vector_Get(m_History, i);
            Memory_Free(prompt);
        }
        Vector_Free(m_History);
        m_History = NULL;
    }
}

int32_t Console_History_GetLength(void)
{
    return m_History->count;
}

void Console_History_Clear(void)
{
    Vector_Clear(m_History);
}

void Console_History_Append(const char *const prompt)
{
    if (m_History->count == MAX_HISTORY_ENTRIES) {
        Vector_RemoveAt(m_History, 0);
    }
    char *prompt_copy = Memory_DupStr(prompt);
    Vector_Add(m_History, &prompt_copy);
}

const char *Console_History_Get(const int32_t idx)
{
    if (idx < 0 || idx >= m_History->count) {
        return NULL;
    }
    const char *const prompt = *(char **)Vector_Get(m_History, idx);
    return prompt;
}
