#include <trx/game/locale/chain.h>

#include <trx/core/memory.h>
#include <trx/core/strings.h>

#include <ctype.h>
#include <string.h>

typedef struct {
    GAME_DYNAMIC_PATH path;
} M_RESOLVE_CONTEXT;

static void M_AddCode(
    LOCALE_CHAIN *const chain, const char *const code, const size_t len)
{
    if (len == 0 || len >= LOCALE_CODE_SIZE
        || chain->count >= LOCALE_CHAIN_MAX) {
        return;
    }
    char *const out = chain->codes[chain->count++];
    for (size_t i = 0; i < len; i++) {
        out[i] = (char)tolower((unsigned char)code[i]);
    }
    out[len] = '\0';
}

static char *M_ResolveFunc(const char *const path, void *const user_data)
{
    const M_RESOLVE_CONTEXT *const ctx = user_data;
    const char *const resolved = GamePath_PeekResolve(ctx->path, path);
    return resolved != nullptr ? Memory_DupStr(resolved) : nullptr;
}

static char *M_MoveIntoFolder(const char *const path, const char *const code)
{
    const char *slash = strrchr(path, '/');
    const char *const backslash = strrchr(path, '\\');
    if (backslash != nullptr && (slash == nullptr || backslash > slash)) {
        slash = backslash;
    }
    if (slash == nullptr) {
        return String_Format("%s/%s", code, path);
    }
    return String_Format("%.*s/%s%s", (int)(slash - path), path, code, slash);
}

LOCALE_CHAIN LocaleChain_FromCode(const char *const code)
{
    LOCALE_CHAIN chain = {};
    if (code == nullptr || code[0] == '\0') {
        return chain;
    }
    M_AddCode(&chain, code, strlen(code));
    const char *const dash = strchr(code, '-');
    if (dash != nullptr) {
        M_AddCode(&chain, code, (size_t)(dash - code));
    }
    return chain;
}

char *LocaleChain_Find(
    const LOCALE_CHAIN *const chain, const char *const path,
    const LOCALE_FIND_FUNC func, void *const user_data)
{
    for (int32_t i = 0; i < chain->count; i++) {
        char *moved = M_MoveIntoFolder(path, chain->codes[i]);
        char *const result = func(moved, user_data);
        Memory_FreePointer(&moved);
        if (result != nullptr) {
            return result;
        }
    }
    return func(path, user_data);
}

char *LocaleChain_Resolve(
    const LOCALE_CHAIN *const chain, const GAME_DYNAMIC_PATH path,
    const char *const rel)
{
    M_RESOLVE_CONTEXT ctx = { .path = path };
    return LocaleChain_Find(chain, rel, M_ResolveFunc, &ctx);
}
