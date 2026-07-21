#include <trx/game/console/registry.h>

#include <trx/core/memory.h>

#include <ctype.h>
#include <string.h>

typedef struct M_NODE {
    CONSOLE_COMMAND cmd;
    struct M_NODE *next;
} M_NODE;

static M_NODE *m_List = nullptr;

__attribute__((destructor)) static void M_Shutdown(void)
{
    M_NODE *current = m_List;

    while (current != nullptr) {
        M_NODE *const next = current->next;
        Memory_Free((char *)current->cmd.prefix);
        Memory_Free((char *)current->cmd.help_id);
        Memory_Free((char *)current->cmd.aliases);
        Memory_Free(current);
        current = next;
    }

    m_List = nullptr;
}

// The command word is the first run of non-space characters.
static size_t M_WordLen(const char *const cmdline)
{
    size_t len = 0;
    while (cmdline[len] != '\0' && isspace((unsigned char)cmdline[len]) == 0) {
        len++;
    }
    return len;
}

static bool M_RangeEquals(
    const char *const word, const size_t word_len, const char *const name,
    const size_t name_len)
{
    if (word_len != name_len) {
        return false;
    }
    for (size_t i = 0; i < word_len; i++) {
        if (tolower((unsigned char)word[i])
            != tolower((unsigned char)name[i])) {
            return false;
        }
    }
    return true;
}

// Aliases arrive comma-joined for display ("keys, guns, moreguns"); each
// spelling also dispatches.
static bool M_AliasesContain(
    const char *const aliases, const char *const word, const size_t word_len)
{
    if (aliases == nullptr) {
        return false;
    }
    const char *p = aliases;
    while (*p != '\0') {
        while (*p == ',' || *p == ' ') {
            p++;
        }
        const char *const start = p;
        while (*p != '\0' && *p != ',') {
            p++;
        }
        const char *end = p;
        while (end > start && end[-1] == ' ') {
            end--;
        }
        if (M_RangeEquals(word, word_len, start, (size_t)(end - start))) {
            return true;
        }
    }
    return false;
}

const CONSOLE_COMMAND *Console_Registry_Get(const char *const cmdline)
{
    const size_t word_len = M_WordLen(cmdline);
    const M_NODE *current = m_List;
    while (current != nullptr) {
        const M_NODE *const next = current->next;
        if (M_RangeEquals(
                cmdline, word_len, current->cmd.prefix,
                strlen(current->cmd.prefix))
            || M_AliasesContain(current->cmd.aliases, cmdline, word_len)) {
            return &current->cmd;
        }
        current = next;
    }
    return nullptr;
}

// The strings are copied: a Lua registration's belong to the Lua state, and the
// registry outlives it.
void Console_Registry_Add(CONSOLE_COMMAND cmd)
{
    M_NODE *node = Memory_Alloc(sizeof(M_NODE));
    node->cmd = cmd;
    node->cmd.prefix = Memory_DupStr(cmd.prefix);
    node->cmd.help_id = Memory_DupStr(cmd.help_id);
    node->cmd.aliases = Memory_DupStr(cmd.aliases);
    node->next = m_List;
    m_List = node;
}

void Console_Registry_RemoveByProc(
    COMMAND_RESULT (*const proc)(const COMMAND_CONTEXT *ctx))
{
    M_NODE **link = &m_List;
    while (*link != nullptr) {
        M_NODE *const node = *link;
        if (node->cmd.proc != proc) {
            link = &node->next;
            continue;
        }
        *link = node->next;
        Memory_Free((char *)node->cmd.prefix);
        Memory_Free((char *)node->cmd.help_id);
        Memory_Free((char *)node->cmd.aliases);
        Memory_Free(node);
    }
}

VECTOR *Console_Registry_GetAll(void)
{
    VECTOR *vec = Vector_Create(sizeof(const CONSOLE_COMMAND *));
    M_NODE *node = m_List;
    while (node != nullptr) {
        const CONSOLE_COMMAND *cmd_ptr = &node->cmd;
        Vector_Add(vec, &cmd_ptr);
        node = node->next;
    }
    return vec;
}
