#include <trx/game/replay/test_replay.h>

#include <trx/config.h>
#include <trx/config/registry.h>
#include <trx/core/enum_map.h>
#include <trx/core/filesystem.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/console/common.h>
#include <trx/game/input/backends/controller.h>
#include <trx/game/input/backends/keyboard.h>
#include <trx/game/input/common.h>
#include <trx/game/input/sdl.h>
#include <trx/game/lara.h>
#include <trx/game/lua.h>
#include <trx/game/random.h>
#include <trx/game/shell.h>
#include <trx/game/shell/events.h>

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define M_DEBUG 0
typedef struct {
    bool seen;
    STARTUP_SETTINGS settings;
} M_STARTUP_SNAPSHOT;

typedef struct {
    SHELL_ARGS *args;
    M_STARTUP_SNAPSHOT startup;
    bool used_deprecated_args;
    // The headers are read twice: once for the startup settings the shell
    // needs before it registers any option, and again once the options exist.
    // Only the second pass has anywhere to put a setting.
    bool apply_config;
} M_PARSE_CTX;

typedef struct {
    bool collecting;
    int32_t brace_depth;
    bool in_quote;
    bool escaped;
} M_BLOCK_EVENT_CTX;

// Parsed frame events
typedef struct {
    int32_t frame_idx;
    VECTOR *events; // vector of char*
} M_FRAME;

// A setting the recording asks for that no option answers to yet, kept until
// the game's script has declared its own.
typedef struct {
    char *key;
    char *value;
} M_DEFERRED_OPTION;

// Replay private state
typedef struct {
    char *data; // Replay file data buffer
    size_t size; // Size of data buffer
    VECTOR *headers; // Vector of char* header lines
    VECTOR *deferred_config; // Vector of M_DEFERRED_OPTION
    VECTOR *frames; // Vector of M_FRAME frames to play
    int32_t frame_idx; // Current playback frame index
    int32_t next_frame_idx; // Next frame to process
    bool replay_quiet;
    bool skipping; // a skip stretch has the drawing off
    struct {
        bool seen;
        bool quiet_applied;
        LOG_LEVEL log_level_before_quiet;
        bool summary_printed;
        bool case_active;
        char *case_name;
        int32_t case_checks;
        int32_t case_fails;
        int32_t cases_passed;
        int32_t cases_failed;
        int32_t checks_passed;
        int32_t checks_failed;
        int32_t exit_code_override;
        bool use_ansi_colors;
    } test_mode;
} M_PRIV;

typedef bool (*M_EVENT_HANDLER)(const char *token);

typedef bool (*M_HEADER_HANDLER)(const char *line, M_PARSE_CTX *ctx);

static M_PRIV m_Priv = {};

// Event parsers
static bool M_ParseQuitEvent(const char *event_str);
static bool M_ParseKeyDownEvent(const char *event_str);
static bool M_ParseKeyUpEvent(const char *event_str);
static bool M_ParseTextInputEvent(const char *event_str);
static bool M_ParseCommandEvent(const char *event_str);
static bool M_ParseNoopEvent(const char *event_str);
static bool M_ParseLuaEvent(const char *event_str);
static bool M_ParseTestCaseEvent(const char *event_str);
static bool M_ParseExpectEvent(const char *event_str);
static bool M_ParseSkipStartEvent(const char *event_str);
static bool M_ParseSkipEndEvent(const char *event_str);

// Header parsers
static bool M_ParseSeedControl(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseSeedDraw(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseStartup(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseBindKeyboard(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseBindController(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseArgs(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseConfig(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseTestCaseHeader(const char *line, M_PARSE_CTX *ctx);

static const M_HEADER_HANDLER m_HeaderHandlers[] = {
    M_ParseSeedControl,  M_ParseSeedDraw,       M_ParseStartup,
    M_ParseBindKeyboard, M_ParseBindController, M_ParseArgs,
    M_ParseConfig,       M_ParseTestCaseHeader, nullptr,
};

static const M_EVENT_HANDLER m_EventHandlers[] = {
    M_ParseQuitEvent,      M_ParseTestCaseEvent, M_ParseExpectEvent,
    M_ParseKeyDownEvent,   M_ParseKeyUpEvent,    M_ParseTextInputEvent,
    M_ParseNoopEvent,      M_ParseCommandEvent,  M_ParseLuaEvent,
    M_ParseSkipStartEvent, M_ParseSkipEndEvent,  nullptr,
};

static inline M_PARSE_CTX M_ParseCtxInit(void)
{
    return (M_PARSE_CTX) {
        .startup.settings = {
            .level_request = { .num = -1 },
            .save_to_load = -1,
        },
    };
}

static void M_TestPrint(const char *const fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    printf("\n");
    fflush(stdout);
    va_end(va);
}

static const char *M_TestColor(const char *const color)
{
    return m_Priv.test_mode.use_ansi_colors ? color : "";
}

static const char *M_TestColorReset(void)
{
    return m_Priv.test_mode.use_ansi_colors ? LOG_ANSI_COLOR_RESET : "";
}

static void M_EndTestCase(void)
{
    M_PRIV *const p = &m_Priv;
    if (!p->test_mode.case_active) {
        return;
    }

    if (p->test_mode.case_fails == 0) {
        p->test_mode.cases_passed++;
        M_TestPrint(
            "%sPASS%s | %s", M_TestColor(LOG_ANSI_COLOR_GREEN),
            M_TestColorReset(), p->test_mode.case_name);
    } else {
        p->test_mode.cases_failed++;
        M_TestPrint(
            "%sFAIL%s | %s (%d/%d checks failed)",
            M_TestColor(LOG_ANSI_COLOR_RED), M_TestColorReset(),
            p->test_mode.case_name, p->test_mode.case_fails,
            p->test_mode.case_checks);
    }

    p->test_mode.case_active = false;
    Memory_FreePointer(&p->test_mode.case_name);
    p->test_mode.case_checks = 0;
    p->test_mode.case_fails = 0;
}

static void M_TestReportSummary(void)
{
    M_PRIV *const p = &m_Priv;
    if (!p->test_mode.seen || p->test_mode.summary_printed) {
        return;
    }

    if (p->test_mode.case_active) {
        M_EndTestCase();
    }

    M_TestPrint("\n=== TEST_SUMMARY ===");
    const int32_t total_cases =
        p->test_mode.cases_passed + p->test_mode.cases_failed;
    if (p->test_mode.cases_passed > 0) {
        M_TestPrint(
            "%sPASSED: %d of %d%s", M_TestColor(LOG_ANSI_COLOR_GREEN),
            p->test_mode.cases_passed, total_cases, M_TestColorReset());
    }
    if (p->test_mode.cases_failed > 0) {
        M_TestPrint(
            "%sFAILED: %d of %d%s", M_TestColor(LOG_ANSI_COLOR_RED),
            p->test_mode.cases_failed, total_cases, M_TestColorReset());
    }
    p->test_mode.summary_printed = true;
}

static void M_ApplyQuietInTestMode(void)
{
    M_PRIV *const p = &m_Priv;
    if (!p->replay_quiet || !p->test_mode.seen || p->test_mode.quiet_applied) {
        return;
    }
    p->test_mode.log_level_before_quiet = Log_GetMinLevel();
    Log_SetMinLevel((LOG_LEVEL)100);
    p->test_mode.quiet_applied = true;
}

static void M_TerminateFromTestResult(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->test_mode.cases_failed > 0 || p->test_mode.checks_failed > 0) {
        p->test_mode.exit_code_override = 1;
    } else {
        p->test_mode.exit_code_override = 0;
    }

    SDL_Event event = { .type = SDL_QUIT };
    Shell_ProcessEvent(&event);
}

static bool M_ParseQuotedPayload(
    const char *const event_str, const char *const prefix,
    const char **const out_start, size_t *const out_len)
{
    if (strncmp(event_str, prefix, strlen(prefix)) != 0) {
        return false;
    }

    const char *const start = strchr(event_str + strlen(prefix), '"');
    const char *const end = start ? strrchr(start + 1, '"') : nullptr;
    if (start == nullptr || end == nullptr || end <= start + 1) {
        return false;
    }

    *out_start = start + 1;
    *out_len = (size_t)(end - (start + 1));
    return true;
}

static void M_FreeStartupSnapshot(M_STARTUP_SNAPSHOT *const startup)
{
    Memory_FreePointer(&startup->settings.level_request.path);
    Memory_FreePointer(&startup->settings.level_request.query);
}

static void M_ResetParseArgs(M_PARSE_CTX *const ctx)
{
    if (ctx->args == nullptr) {
        return;
    }
    Shell_FreeArgs(ctx->args);
    ctx->args = nullptr;
}

static SHELL_ARGS *M_BuildArgsFromStartupSnapshot(M_PARSE_CTX *const ctx)
{
    if (!ctx->startup.seen) {
        return nullptr;
    }

    SHELL_ARGS *const args = Memory_Alloc(sizeof(SHELL_ARGS));
    args->startup = ctx->startup.settings;
    ctx->startup.settings.level_request.path = nullptr;
    ctx->startup.settings.level_request.query = nullptr;

    // A level named by path plays through the direct level mod, the same as
    // --level does, so a recording that names a game as well still plays the
    // file it names rather than that game's first level.
    if (args->startup.level_request.path != nullptr) {
        const SHELL_MOD *const direct_mod =
            Shell_GetModByType(MOD_DIRECT_LEVEL, args->startup.engine_version);
        if (direct_mod != nullptr) {
            args->startup.mod = direct_mod;
        }
    }

    if (args->startup.mod == nullptr) {
        if (args->startup.level_request.path != nullptr) {
            args->startup.mod = Shell_GetModByType(
                MOD_DIRECT_LEVEL, args->startup.engine_version);
        } else if (args->startup.level_request.query != nullptr) {
            args->startup.mod =
                Shell_GetModByType(MOD_BASE_GAME, args->startup.engine_version);
        } else if (args->startup.engine_version > 0) {
            args->startup.mod =
                Shell_GetModByType(MOD_BASE_GAME, args->startup.engine_version);
        }
    }
    return args;
}

static const char *M_SkipWhitespaceConst(const char *const s)
{
    const char *p = s;
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

static char *M_TrimWhitespaceInPlace(char *const s)
{
    char *start = s;
    while (*start == ' ' || *start == '\t' || *start == '\n'
           || *start == '\r') {
        start++;
    }
    char *end = start + strlen(start);
    while (end > start
           && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\n'
               || end[-1] == '\r')) {
        end--;
    }
    *end = '\0';
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }
    return s;
}

static void M_ScanBraceState(
    const char *const s, int32_t *const io_brace_depth, bool *const io_in_quote,
    bool *const io_escaped)
{
    for (const char *p = s; *p != '\0'; p++) {
        if (*io_in_quote) {
            if (*io_escaped) {
                *io_escaped = false;
                continue;
            }
            if (*p == '\\') {
                *io_escaped = true;
                continue;
            }
            if (*p == '"') {
                *io_in_quote = false;
            }
            continue;
        }

        if (*p == '"') {
            *io_in_quote = true;
            continue;
        }
        if (*p == '{') {
            (*io_brace_depth)++;
            continue;
        }
        if (*p == '}') {
            (*io_brace_depth)--;
            continue;
        }
    }
}

static const char *M_GetBlockPayloadStartIfAny(const char *const evt)
{
    if (strncmp(evt, "expect ", strlen("expect ")) == 0) {
        return M_SkipWhitespaceConst(evt + strlen("expect "));
    }
    if (strncmp(evt, "lua ", strlen("lua ")) == 0) {
        return M_SkipWhitespaceConst(evt + strlen("lua "));
    }
    if (strncmp(evt, "cmd ", strlen("cmd ")) == 0) {
        return M_SkipWhitespaceConst(evt + strlen("cmd "));
    }
    return nullptr;
}

static bool M_TryStartBlockEvent(
    M_BLOCK_EVENT_CTX *const ctx, const char *const evt)
{
    const char *const payload_start = M_GetBlockPayloadStartIfAny(evt);
    if (payload_start == nullptr || *payload_start != '{') {
        return false;
    }

    ctx->collecting = true;
    ctx->brace_depth = 0;
    ctx->in_quote = false;
    ctx->escaped = false;
    M_ScanBraceState(
        payload_start, &ctx->brace_depth, &ctx->in_quote, &ctx->escaped);
    if (ctx->brace_depth == 0) {
        ctx->collecting = false;
    }
    return true;
}

static bool M_GetBracedPayload(
    const char *const event_str, const char *const prefix,
    const char **const out_start, size_t *const out_len)
{
    if (strncmp(event_str, prefix, strlen(prefix)) != 0) {
        return false;
    }

    const char *p = M_SkipWhitespaceConst(event_str + strlen(prefix));
    if (*p != '{') {
        return false;
    }

    const char *payload_start = p + 1;
    int32_t depth = 1;
    bool in_quote = false;
    bool escaped = false;
    p++;
    for (; *p != '\0'; p++) {
        if (in_quote) {
            if (escaped) {
                escaped = false;
                continue;
            }
            if (*p == '\\') {
                escaped = true;
                continue;
            }
            if (*p == '"') {
                in_quote = false;
            }
            continue;
        }

        if (*p == '"') {
            in_quote = true;
            continue;
        }
        if (*p == '{') {
            depth++;
            continue;
        }
        if (*p == '}') {
            depth--;
            if (depth == 0) {
                const char *trail = M_SkipWhitespaceConst(p + 1);
                if (*trail != '\0') {
                    return false;
                }
                *out_start = payload_start;
                *out_len = (size_t)(p - payload_start);
                return true;
            }
        }
    }
    return false;
}

static bool M_ParseQuitEvent(const char *const event_str)
{
    if (strcmp(event_str, "quit") != 0) {
        return false;
    }

    if (m_Priv.test_mode.seen) {
        M_TestReportSummary();
        M_TerminateFromTestResult();
    }

    SDL_Event event = { .type = SDL_QUIT };
    Shell_ProcessEvent(&event);
    return true;
}

// Consolidate keydown/keyup parsing into a single helper
static bool M_ParseKeyEvent(
    const char *event_str, SDL_EventType type, const char *prefix)
{
    if (strncmp(event_str, prefix, strlen(prefix)) != 0) {
        return false;
    }
    SDL_Event event = { .type = type };
    const char *p = event_str + strlen(prefix);
    const char *start = strchr(p, '"');
    const char *end = start ? strrchr(start + 1, '"') : nullptr;
    if (!start || !end || end <= start + 1) {
        LOG_WARNING("Malformed %s instruction: %s", prefix, event_str);
        return false;
    }
    const size_t slen = end - (start + 1);
    const char *desc = String_FormatStatic("%.*s", slen, start + 1);
    SDL_Keymod mod;
    if (!Input_ParseKeyDesc(desc, &event.key.keysym.scancode, &mod)) {
        return false;
    }
    event.key.keysym.mod = mod;
    event.key.keysym.sym = SDL_GetKeyFromScancode(event.key.keysym.scancode);
    Shell_ProcessEvent(&event);
    return true;
}

static bool M_ParseKeyDownEvent(const char *event_str)
{
    return M_ParseKeyEvent(event_str, SDL_KEYDOWN, "●");
}

static bool M_ParseKeyUpEvent(const char *event_str)
{
    return M_ParseKeyEvent(event_str, SDL_KEYUP, "○");
}

static bool M_ParseTextInputEvent(const char *const event_str)
{
    SDL_Event event = { .type = SDL_TEXTINPUT };
    const char *const fmt = String_FormatStatic(
        "text-input \"%%%d[^\"]\"", SDL_TEXTEDITINGEVENT_TEXT_SIZE - 1);
    if (sscanf(event_str, fmt, &event.text.text) != 1) {
        return false;
    }
    Shell_ProcessEvent(&event);
    return true;
}

static bool M_ParseNoopEvent(const char *const event_str)
{
    // No-op event for inline comments and empty frame markers
    if (strncmp(event_str, "noop", 4) != 0) {
        return false;
    }
    return true;
}

static void M_StopSkipping(void)
{
    M_PRIV *const p = &m_Priv;
    if (!p->skipping) {
        return;
    }
    p->skipping = false;
    Shell_SetHeadless(false);
}

static bool M_ParseSkipStartEvent(const char *const event_str)
{
    if (strcmp(event_str, "skip start") != 0) {
        return false;
    }
    M_PRIV *const p = &m_Priv;
    // A run already told to draw nothing must not start drawing at skip end.
    if (!p->skipping && !Shell_GetArgs()->headless) {
        p->skipping = true;
        Shell_SetHeadless(true);
    }
    return true;
}

static bool M_ParseSkipEndEvent(const char *const event_str)
{
    if (strcmp(event_str, "skip end") != 0) {
        return false;
    }
    M_StopSkipping();
    return true;
}

static bool M_ParseTestCaseEvent(const char *const event_str)
{
    M_PRIV *const p = &m_Priv;
    const char *name_start = nullptr;
    size_t name_len = 0;
    if (!M_ParseQuotedPayload(event_str, "testcase ", &name_start, &name_len)) {
        return false;
    }

    p->test_mode.seen = true;
    M_ApplyQuietInTestMode();

    if (p->test_mode.case_active) {
        M_EndTestCase();
    }

    Memory_FreePointer(&p->test_mode.case_name);
    p->test_mode.case_name = String_Format("%.*s", (int)name_len, name_start);
    p->test_mode.case_checks = 0;
    p->test_mode.case_fails = 0;
    p->test_mode.case_active = true;
    return true;
}

static bool M_ParseTestCaseHeader(const char *const line, M_PARSE_CTX *const)
{
    return M_ParseTestCaseEvent(line);
}

static bool M_ParseExpectEvent(const char *const event_str)
{
    M_PRIV *const p = &m_Priv;
    const char *const prefix = "expect ";
    if (strncmp(event_str, prefix, strlen(prefix)) != 0) {
        return false;
    }
    const char *expr_start = nullptr;
    size_t expr_len = 0;
    if (!M_GetBracedPayload(event_str, prefix, &expr_start, &expr_len)) {
        expr_start = M_SkipWhitespaceConst(event_str + strlen(prefix));
        if (*expr_start == '\0') {
            return false;
        }
        expr_len = strlen(expr_start);
    }

    p->test_mode.seen = true;
    M_ApplyQuietInTestMode();

    if (!p->test_mode.case_active) {
        M_TestPrint(
            "%sFAIL%s | expect outside test case",
            M_TestColor(LOG_ANSI_COLOR_RED), M_TestColorReset());
        p->test_mode.checks_failed++;
        p->test_mode.cases_failed++;
        return true;
    }

    char *const expr = String_Format("%.*s", (int)expr_len, expr_start);
    char *const script = String_Format(
        "if not ((function() return %s\n end)()) then error('expect failed') "
        "end",
        expr);
    LUA_RESULT eval_result = LUA_Eval(script);

    p->test_mode.case_checks++;
    if (eval_result.code == LUA_OK) {
        p->test_mode.checks_passed++;
    } else {
        p->test_mode.checks_failed++;
        p->test_mode.case_fails++;
        M_TestPrint(
            "%sFAIL%s | %s | expect \"%s\" | %s",
            M_TestColor(LOG_ANSI_COLOR_RED), M_TestColorReset(),
            p->test_mode.case_name, expr,
            eval_result.message != nullptr ? eval_result.message : "lua error");
    }

    LUA_FreeResult(&eval_result);
    Memory_Free(script);
    Memory_Free(expr);
    return true;
}

static bool M_ParseCommandEvent(const char *const event_str)
{
    const char *const prefix = "cmd ";
    if (strncmp(event_str, prefix, strlen(prefix)) != 0) {
        return false;
    }

    const char *payload_start = nullptr;
    size_t payload_len = 0;

    if (M_GetBracedPayload(event_str, prefix, &payload_start, &payload_len)) {
        char *const cmd_str =
            String_Format("%.*s", (int)payload_len, payload_start);
        M_TrimWhitespaceInPlace(cmd_str);
        Console_Eval(cmd_str);
        Memory_Free(cmd_str);
        return true;
    }

    if (M_ParseQuotedPayload(event_str, prefix, &payload_start, &payload_len)) {
        char *const cmd_str =
            String_Format("%.*s", (int)payload_len, payload_start);
        Console_Eval(cmd_str);
        Memory_Free(cmd_str);
        return true;
    }

    payload_start = M_SkipWhitespaceConst(event_str + strlen(prefix));
    payload_len = strlen(payload_start);
    if (payload_len == 0) {
        LOG_WARNING("Malformed cmd instruction: %s", event_str);
        return false;
    }

    char *const cmd_str =
        String_Format("%.*s", (int)payload_len, payload_start);
    M_TrimWhitespaceInPlace(cmd_str);
    if (cmd_str[0] == '\0') {
        LOG_WARNING("Malformed cmd instruction: %s", event_str);
        Memory_Free(cmd_str);
        return false;
    }
    Console_Eval(cmd_str);
    Memory_Free(cmd_str);
    return true;
}

static bool M_ParseLuaEvent(const char *const event_str)
{
    M_PRIV *const p = &m_Priv;
    if (strncmp(event_str, "lua ", 4) != 0) {
        return false;
    }

    const char *chunk_start = nullptr;
    size_t chunk_len = 0;
    LUA_RESULT eval_result = {};
    if (M_GetBracedPayload(event_str, "lua ", &chunk_start, &chunk_len)) {
        char *const chunk = String_Format("%.*s", (int)chunk_len, chunk_start);
        eval_result = LUA_Eval(chunk);
        Memory_Free(chunk);
    } else {
        eval_result = LUA_Eval(event_str + 4);
    }
    if (eval_result.code == LUA_ERRSYNTAX) {
        LOG_ERROR(
            "LUA syntax error on frame %d: %s", p->frame_idx,
            eval_result.message);
        Shell_Terminate(1);
    } else if (eval_result.code != LUA_OK) {
        LOG_ERROR(
            "LUA error on frame %d: %s", p->frame_idx, eval_result.message);
        Shell_Terminate(1);
    }
    LUA_FreeResult(&eval_result);
    return true;
}

static bool M_ParseEvent(const char *const event_str)
{
    for (int32_t i = 0; m_EventHandlers[i] != nullptr; i++) {
        if (m_EventHandlers[i](event_str)) {
            return true;
        }
    }
    return false;
}

static bool M_ParseSeedControl(const char *const line, M_PARSE_CTX *const ctx)
{
    int32_t val;
    if (sscanf(line, "seed_control %d", &val) == 1) {
        Random_SeedControl(val);
        return true;
    }
    return false;
}

static bool M_ParseSeedDraw(const char *const line, M_PARSE_CTX *const ctx)
{
    int32_t val;
    if (sscanf(line, "seed_draw %d", &val) == 1) {
        Random_SeedDraw(val);
        return true;
    }
    return false;
}

static bool M_ParseBindKeyboard(const char *const line, M_PARSE_CTX *const ctx)
{
    const char *prefix = "bind keyboard ";
    if (strncmp(line, prefix, strlen(prefix)) != 0) {
        return false;
    }
    const char *p = line + strlen(prefix);
    const char *q = strchr(p, ' ');
    if (q == nullptr) {
        return false;
    }
    const char *role_str = String_FormatStatic("%.*s", (int)(q - p), p);
    const INPUT_ROLE role = ENUM_MAP_GET(INPUT_ROLE, role_str, -1);
    if (role == (INPUT_ROLE)-1) {
        return false;
    }
    const char *const start = strchr(p, '"');
    const char *const end = start ? strrchr(start + 1, '"') : nullptr;
    if (start == nullptr || end == nullptr || end < start + 1) {
        LOG_WARNING("Malformed bind keyboard instruction: %s", line);
        return false;
    }
    const size_t slen = end - (start + 1);
    const char *desc = String_FormatStatic("%.*s", slen, start + 1);
    SDL_Scancode sc = SDL_SCANCODE_UNKNOWN;
    SDL_Keymod mod = KMOD_NONE;
    if (desc[0] != '\0') {
        if (!Input_ParseKeyDesc(desc, &sc, &mod)) {
            return false;
        }
    }
    JSON_OBJECT *const bind = JSON_ObjectNew();
    JSON_ObjectAppendInt(bind, "scancode", sc);
    JSON_ObjectAppendInt(bind, "mod", mod);
    g_Input_Keyboard.assign_from_json_object(
        g_Config.input.keyboard_layout, role, 0, bind);
    JSON_ObjectFree(bind);
    return true;
}

static bool M_ParseBindController(
    const char *const line, M_PARSE_CTX *const ctx)
{
    const char *prefix = "bind controller ";
    if (strncmp(line, prefix, strlen(prefix)) != 0) {
        return false;
    }
    const char *p = line + strlen(prefix);
    const char *q = strchr(p, ' ');
    if (q == nullptr) {
        return false;
    }
    const char *role_str = String_FormatStatic("%.*s", (int)(q - p), p);
    const INPUT_ROLE role =
        (INPUT_ROLE)ENUM_MAP_GET(INPUT_ROLE, role_str, (int32_t)(INPUT_ROLE)-1);
    if (role == (INPUT_ROLE)-1) {
        return false;
    }
    int32_t bt, b, ad;
    if (sscanf(q + 1, "%d %d %d", &bt, &b, &ad) == 3) {
        JSON_OBJECT *bind = JSON_ObjectNew();
        JSON_ObjectAppendInt(bind, "button_type", bt);
        JSON_ObjectAppendInt(bind, "bind", b);
        JSON_ObjectAppendInt(bind, "axis_dir", ad);
        g_Input_Controller.assign_from_json_object(
            g_Config.input.controller_layout, role, 0, bind);
        JSON_ObjectFree(bind);
        return true;
    }
    return false;
}

static bool M_ParseStartup(const char *const line, M_PARSE_CTX *const ctx)
{
    if (strncmp(line, "startup ", 8) != 0) {
        return false;
    }

    int32_t int_val = 0;
    const char *str = nullptr;
    size_t str_len = 0;

    if (sscanf(line, "startup engine %d", &int_val) == 1) {
        ctx->startup.settings.engine_version = int_val;
    } else if (M_ParseQuotedPayload(line, "startup mod", &str, &str_len)) {
        const char *const mod_name =
            String_FormatStatic("%.*s", (int)str_len, str);
        ctx->startup.settings.mod = Shell_GetModByName(mod_name);
        if (ctx->startup.settings.mod == nullptr) {
            LOG_ERROR("the replay names a game that is not here: %s", mod_name);
            return false;
        }
        if (ctx->startup.settings.engine_version <= 0) {
            ctx->startup.settings.engine_version =
                ctx->startup.settings.mod->engine_version;
        }
    } else if (sscanf(line, "startup level-num %d", &int_val) == 1) {
        ctx->startup.settings.level_request.num = int_val;
    } else if (
        M_ParseQuotedPayload(line, "startup level-query", &str, &str_len)) {
        Memory_FreePointer(&ctx->startup.settings.level_request.query);
        ctx->startup.settings.level_request.query =
            Memory_DupStr(String_FormatStatic("%.*s", (int)str_len, str));
    } else if (
        M_ParseQuotedPayload(line, "startup level-path", &str, &str_len)) {
        Memory_FreePointer(&ctx->startup.settings.level_request.path);
        ctx->startup.settings.level_request.path =
            Memory_DupStr(String_FormatStatic("%.*s", (int)str_len, str));
    } else if (sscanf(line, "startup save %d", &int_val) == 1) {
        ctx->startup.settings.save_to_load = int_val - 1;
    } else {
        return false;
    }

    ctx->startup.seen = true;
    return true;
}

static bool M_ParseArgs(const char *const line, M_PARSE_CTX *const ctx)
{
    if (strncmp(line, "args", 4) != 0) {
        return false;
    }

    ctx->used_deprecated_args = true;
    M_ResetParseArgs(ctx);

    // Build an owned argv vector for Shell_ParseArgs adoption.
    VECTOR *raw_args = Vector_Create(sizeof(const char *));
    const char *p = line + 4;
    while (*p != '\0') {
        while (isspace((unsigned char)*p)) {
            p++;
        }
        if (*p != '"') {
            break;
        }

        p++;
        const char *start = p;
        while (*p != '\0' && *p != '"') {
            p++;
        }
        const ptrdiff_t len = p - start;
        char tmp[len + 1];
        memcpy(tmp, start, len);
        tmp[len] = '\0';
        char *arg = Memory_DupStr(tmp);
        Vector_Add(raw_args, &arg);
        if (*p == '"') {
            p++;
        }
    }
    if (!Result_Absorb(Shell_ParseArgs(raw_args, &ctx->args))) {
        return false;
    }
    return true;
}

static bool M_ParseConfig(const char *const line, M_PARSE_CTX *const ctx)
{
    char keybuf[64];
    char valbuf[128];
    if (sscanf(line, "config %63s %127s", keybuf, valbuf) == 2) {
        if (!ctx->apply_config) {
            return true;
        }
        // Strip surrounding quotes from the value, if present
        size_t vlen = strlen(valbuf);
        if (vlen >= 2 && valbuf[0] == '"' && valbuf[vlen - 1] == '"') {
            valbuf[vlen - 1] = '\0';
            memmove(valbuf, valbuf + 1, vlen - 1);
        }
        CONFIG_OPTION *opt = Config_FindOption(keybuf);
        if (opt != nullptr) {
            Config_Option_SetFromString(opt, valbuf, false);
            return true;
        }
        // A game declares its own settings as its script runs, which is after
        // the header is read. The name is kept rather than refused, and tried
        // again once those options exist.
        M_PRIV *const p = &m_Priv;
        if (p->deferred_config == nullptr) {
            p->deferred_config = Vector_Create(sizeof(M_DEFERRED_OPTION));
        }
        Vector_Add(
            p->deferred_config,
            &(M_DEFERRED_OPTION) {
                .key = Memory_DupStr(keybuf),
                .value = Memory_DupStr(valbuf),
            });
        return true;
    }
    return false;
}

static void M_StripInlineComment(char *const line)
{
    bool in_quote = false;
    char *p;
    for (p = line; *p != '\0'; p++) {
        if (*p == '"') {
            in_quote = !in_quote;
        } else if (*p == '#' && !in_quote) {
            *p = '\0';
            break;
        }
    }
    // Trim trailing whitespace
    {
        char *end = line + strlen(line);
        while (end > line && (end[-1] == ' ' || end[-1] == '\t')) {
            end[-1] = '\0';
            end--;
        }
    }
}

static char *M_SkipWhitespace(char *const line)
{
    char *start = line;
    while (*start == ' ' || *start == '\t') {
        start++;
    }
    return start;
}

static char *M_SkipUTF8BOM(char *const line)
{
    if (strlen(line) < 3) {
        return line;
    }

    const unsigned char *const bytes = (const unsigned char *)line;
    if (bytes[0] == 0xEF && bytes[1] == 0xBB && bytes[2] == 0xBF) {
        return line + 3;
    }
    return line;
}

static bool M_IsFrameMarkerLine(const char *const line)
{
    int32_t delta = 0;
    return sscanf(line, "@+%d:", &delta) == 1;
}

SHELL_ARGS *TestReplay_Open(const char *path)
{
    M_PRIV *const p = &m_Priv;

    Memory_FreePointer(&p->test_mode.case_name);
    memset(p, 0, sizeof(m_Priv));
    p->replay_quiet = Log_GetMinLevel() >= LOG_LEVEL_WARNING;
    p->test_mode.log_level_before_quiet = Log_GetMinLevel();
    p->test_mode.use_ansi_colors = Log_ShouldUseAnsiColors();
    p->test_mode.exit_code_override = -1;

    char *data = nullptr;
    size_t size = 0;
    if (!SHOULD(FS_Load(path, &data, &size))) {
        return nullptr;
    }
    p->data = data;
    p->size = size;

    // Split file into lines by replacing '\n' with '\0'
    char *end = data + size;
    for (char *ch = data; ch < end; ch++) {
        if (*ch == '\n') {
            *ch = '\0';
        }
    }

    // Collect non-empty, comment-stripped lines
    VECTOR *lines = Vector_Create(sizeof(char *));
    for (char *line = data; line < end;) {
        char *const next_line = line + strlen(line) + 1;
        M_StripInlineComment(line);
        char *start = M_SkipWhitespace(line);
        start = M_SkipUTF8BOM(start);
        start = M_SkipWhitespace(start);
        if (*start != '\0') {
            Vector_Add(lines, &start);
        }
        line = next_line;
    }

    // Parse and execute headers
    p->headers = Vector_Create(sizeof(char *));
    int32_t idx = 0;
    while (idx < lines->count) {
        char *const ln = *(char **)Vector_Get(lines, idx);
        int32_t delta = 0;
        if (sscanf(ln, "@+%d:", &delta) == 1) {
            break;
        }
        Vector_Add(p->headers, &ln);
        idx++;
    }

    // Parse frames and their events
    p->frames = Vector_Create(sizeof(M_FRAME));
    p->next_frame_idx = 0;
    p->frame_idx = 0;
    int32_t last_frame = 0;
    while (idx < lines->count) {
        char *ln = *(char **)Vector_Get(lines, idx);
        int32_t delta = 0;
        if (sscanf(ln, "@+%d:", &delta) == 1) {
            M_FRAME frame = {
                .frame_idx = last_frame + delta,
                .events = Vector_Create(sizeof(char *)),
            };
            M_BLOCK_EVENT_CTX block_ctx = {};

            // Primary event on same line
            char *const colon = strchr(ln, ':');
            if (colon != nullptr) {
                char *const evt = M_SkipWhitespace(colon + 1);
                if (*evt != '\0') {
                    Vector_Add(frame.events, &evt);
                    M_TryStartBlockEvent(&block_ctx, evt);
                }
            }

            // Continued events
            idx++;
            while (idx < lines->count) {
                char *const cont = *(char **)Vector_Get(lines, idx);
                if (M_IsFrameMarkerLine(cont)) {
                    // Reached next frame - stop
                    break;
                }

                char *const evt = M_SkipWhitespace(cont);
                if (*evt == '\0') {
                    idx++;
                    continue;
                }

                if (block_ctx.collecting) {
                    if (evt > p->data) {
                        evt[-1] = '\n';
                    }
                    M_ScanBraceState(
                        evt, &block_ctx.brace_depth, &block_ctx.in_quote,
                        &block_ctx.escaped);
                    if (block_ctx.brace_depth == 0) {
                        block_ctx.collecting = false;
                    }
                    idx++;
                    continue;
                }

                Vector_Add(frame.events, &evt);
                M_TryStartBlockEvent(&block_ctx, evt);
                idx++;
            }
            Vector_Add(p->frames, &frame);
            last_frame = frame.frame_idx;
            continue;
        }
        idx++;
    }

    M_PARSE_CTX ctx = M_ParseCtxInit();
    for (int32_t i = 0; i < p->headers->count; i++) {
        const char *const ln = *(const char **)Vector_Get(p->headers, i);
        for (int32_t j = 0; m_HeaderHandlers[j]; j++) {
            if (m_HeaderHandlers[j](ln, &ctx)) {
                break;
            }
        }
    }
    Vector_Free(lines);
    LOG_INFO("Loaded %zu frames for playback", p->frames->count);
    if (ctx.startup.seen) {
        M_ResetParseArgs(&ctx);
        ctx.args = M_BuildArgsFromStartupSnapshot(&ctx);
    } else if (ctx.args == nullptr) {
        IGNORE(Shell_ParseArgs(nullptr, &ctx.args));
    }
    if (ctx.used_deprecated_args && !ctx.startup.seen) {
        LOG_WARNING(
            "Replay uses deprecated 'args' startup header; please re-record "
            "it");
    }
    if (ctx.args != nullptr && ctx.args->quiet) {
        p->replay_quiet = true;
    }
    M_FreeStartupSnapshot(&ctx.startup);
    return ctx.args;
}

// The settings the recording named that no option answered to when the header
// was read. A game's script has run by now, so its own settings are here.
void TestReplay_ApplyDeferredConfig(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->deferred_config == nullptr) {
        return;
    }
    for (int32_t i = 0; i < p->deferred_config->count; i++) {
        M_DEFERRED_OPTION *const deferred = Vector_Get(p->deferred_config, i);
        CONFIG_OPTION *const option = Config_FindOption(deferred->key);
        if (option == nullptr) {
            LOG_WARNING("Unknown option: %s", deferred->key);
        } else {
            Config_Option_SetFromString(option, deferred->value, false);
        }
        Memory_FreePointer(&deferred->key);
        Memory_FreePointer(&deferred->value);
    }
    Vector_Free(p->deferred_config);
    p->deferred_config = nullptr;
    // Announced rather than discarded as in TestReplay_Start: the game's script
    // has run, so a trx.config.on_change watcher is already attached and was
    // told the option's default. It hears the recorded value from here.
    Config_Update();
}

void TestReplay_Start(void)
{
    M_PARSE_CTX ctx = M_ParseCtxInit();
    ctx.apply_config = true;
    M_PRIV *const p = &m_Priv;
    for (int32_t i = 0; i < p->headers->count; i++) {
        const char *const ln = *(const char **)Vector_Get(p->headers, i);
        bool handled = false;
        for (int32_t j = 0; m_HeaderHandlers[j]; j++) {
            if (m_HeaderHandlers[j](ln, &ctx)) {
                handled = true;
                break;
            }
        }
        if (!handled) {
            LOG_WARNING("Unknown line: %s", ln);
        }
    }
    // The settings the recording asked for are where this replay starts, not
    // something that moved while it ran.
    Config_DiscardPendingChanges();
    Shell_FreeArgs(ctx.args);
    M_FreeStartupSnapshot(&ctx.startup);
}

void TestReplay_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_TestReportSummary();
    M_StopSkipping();

    if (p->test_mode.quiet_applied) {
        Log_SetMinLevel(p->test_mode.log_level_before_quiet);
        p->test_mode.quiet_applied = false;
    }

    Memory_FreePointer(&p->test_mode.case_name);
    if (p->headers) {
        Vector_Free(p->headers);
        p->headers = nullptr;
    }
    if (p->deferred_config != nullptr) {
        for (int32_t i = 0; i < p->deferred_config->count; i++) {
            M_DEFERRED_OPTION *const deferred =
                Vector_Get(p->deferred_config, i);
            Memory_FreePointer(&deferred->key);
            Memory_FreePointer(&deferred->value);
        }
        Vector_Free(p->deferred_config);
        p->deferred_config = nullptr;
    }
    if (p->frames) {
        for (int32_t i = 0; i < p->frames->count; i++) {
            M_FRAME *const f = Vector_Get(p->frames, i);
            Vector_Free(f->events);
        }
        Vector_Free(p->frames);
        p->frames = nullptr;
    }
    if (p->data) {
        Memory_Free(p->data);
        p->data = nullptr;
    }
}

bool TestReplay_IsOpened(void)
{
    M_PRIV *const p = &m_Priv;
    return p->frames != nullptr;
}

void TestReplay_RunFrame(void)
{
    M_PRIV *const p = &m_Priv;
    if (!TestReplay_IsOpened()) {
        return;
    }
    while (p->next_frame_idx < p->frames->count) {
        M_FRAME *const f = Vector_Get(p->frames, p->next_frame_idx);
        if (f->frame_idx != p->frame_idx) {
            break;
        }
        for (int32_t j = 0; j < f->events->count; j++) {
            const char *const evt = *(char **)Vector_Get(f->events, j);
            if (!M_ParseEvent(evt)) {
                LOG_WARNING(
                    "Unknown replay event on frame %d: %s", p->frame_idx, evt);
            }
        }
        p->next_frame_idx++;
    }
    p->frame_idx++;

    if (p->test_mode.seen && p->next_frame_idx >= p->frames->count) {
        M_TestReportSummary();
        M_TerminateFromTestResult();
    }
}

int32_t TestReplay_GetExitCodeOverride(void)
{
    return m_Priv.test_mode.exit_code_override;
}
