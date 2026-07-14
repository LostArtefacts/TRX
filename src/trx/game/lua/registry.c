#include <trx/game/lua/registry.h>

#include <trx/core/memory.h>

typedef struct M_NODE {
    LUA_CAPI capi;
    struct M_NODE *next;
} M_NODE;

static M_NODE *m_List = nullptr;

__attribute__((destructor)) static void M_Shutdown(void)
{
    M_NODE *current = m_List;
    while (current != nullptr) {
        M_NODE *const next = current->next;
        Memory_Free(current);
        current = next;
    }
    m_List = nullptr;
}

void LUA_Registry_Add(LUA_CAPI capi)
{
    M_NODE *const node = Memory_Alloc(sizeof(M_NODE));
    node->capi = capi;
    node->next = m_List;
    m_List = node;
}

void LUA_Registry_CreateAll(lua_State *const L)
{
    for (const M_NODE *node = m_List; node != nullptr; node = node->next) {
        node->capi.create(L);
    }
}

void LUA_Registry_ShutdownAll(void)
{
    for (const M_NODE *node = m_List; node != nullptr; node = node->next) {
        if (node->capi.shutdown != nullptr) {
            node->capi.shutdown();
        }
    }
}
