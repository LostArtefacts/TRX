#include <trx/game/console/registry.h>

#include <trx/core/memory.h>
#include <trx/core/subsystem.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef struct M_NODE {
    CONSOLE_COMMAND cmd;
    struct M_NODE *next;
} M_NODE;

static M_NODE *m_List = nullptr;

// A backstop: the Lua state closes ahead of the subsystems and clears the
// commands it registered, but a session that never opened it still leaves the
// list behind.
static void M_Shutdown(void)
{
    Console_Registry_Clear();
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

// Aliases arrive comma-joined for display ("keys, guns, moreguns"). Advances
// `*cursor` over the next spelling, sets `*len` to its trimmed length, and
// returns its start, or nullptr once the list is spent.
static const char *M_NextAlias(const char **const cursor, size_t *const len)
{
    const char *p = *cursor;
    while (*p == ',' || *p == ' ') {
        p++;
    }
    if (*p == '\0') {
        return nullptr;
    }
    const char *const start = p;
    while (*p != '\0' && *p != ',') {
        p++;
    }
    *cursor = p;
    const char *end = p;
    while (end > start && end[-1] == ' ') {
        end--;
    }
    *len = (size_t)(end - start);
    return start;
}

static bool M_AliasesContain(
    const char *const aliases, const char *const word, const size_t word_len)
{
    if (aliases == nullptr) {
        return false;
    }
    const char *cursor = aliases;
    size_t len;
    for (const char *a; (a = M_NextAlias(&cursor, &len)) != nullptr;) {
        if (M_RangeEquals(word, word_len, a, len)) {
            return true;
        }
    }
    return false;
}

// Whether the nul-terminated `s` equals `name`'s first `name_len` bytes,
// case-insensitively, with nothing past them.
static bool M_NameEqualsN(
    const char *const s, const char *const name, const size_t name_len)
{
    for (size_t i = 0; i < name_len; i++) {
        if (s[i] == '\0'
            || tolower((unsigned char)s[i])
                != tolower((unsigned char)name[i])) {
            return false;
        }
    }
    return s[name_len] == '\0';
}

static int M_CompareNames(const void *const a, const void *const b)
{
    const char *sa = ((const SUGGESTION *)a)->text;
    const char *sb = ((const SUGGESTION *)b)->text;
    for (;; sa++, sb++) {
        const int ca = tolower((unsigned char)*sa);
        const int cb = tolower((unsigned char)*sb);
        if (ca != cb) {
            return ca - cb;
        }
        if (ca == 0) {
            return 0;
        }
    }
}

static void M_AddSuggestion(
    COMPLETION *const out, const char *const name, const size_t name_len,
    const char *const prefix, const size_t prefix_len)
{
    if (name_len < prefix_len) {
        return;
    }
    for (size_t i = 0; i < prefix_len; i++) {
        if (tolower((unsigned char)name[i])
            != tolower((unsigned char)prefix[i])) {
            return;
        }
    }
    for (int32_t i = 0; i < out->suggestions->count; i++) {
        const SUGGESTION *const s = Vector_Get(out->suggestions, i);
        if (M_NameEqualsN(s->text, name, name_len)) {
            return;
        }
    }
    Completion_AddN(out, name, name_len);
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

void Console_Registry_Suggest(
    const char *const word_prefix, COMPLETION *const out)
{
    const size_t prefix_len = strlen(word_prefix);
    for (const M_NODE *node = m_List; node != nullptr; node = node->next) {
        const CONSOLE_COMMAND *const cmd = &node->cmd;
        M_AddSuggestion(
            out, cmd->prefix, strlen(cmd->prefix), word_prefix, prefix_len);
        const char *cursor = cmd->aliases;
        if (cursor == nullptr) {
            continue;
        }
        size_t len;
        for (const char *a; (a = M_NextAlias(&cursor, &len)) != nullptr;) {
            M_AddSuggestion(out, a, len, word_prefix, prefix_len);
        }
    }
    if (out->suggestions->count > 1) {
        qsort(
            Vector_GetData(out->suggestions), out->suggestions->count,
            sizeof(SUGGESTION), M_CompareNames);
    }
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

void Console_Registry_Clear(void)
{
    M_NODE *node = m_List;
    while (node != nullptr) {
        M_NODE *const next = node->next;
        Memory_Free((char *)node->cmd.prefix);
        Memory_Free((char *)node->cmd.help_id);
        Memory_Free((char *)node->cmd.aliases);
        Memory_Free(node);
        node = next;
    }
    m_List = nullptr;
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

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
