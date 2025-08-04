#include "game/test_replay.h"

#include "config.h"
#include "debug.h"
#include "enum_map.h"
#include "filesystem.h"
#include "game/console/common.h"
#include "game/input/backends/controller.h"
#include "game/input/backends/keyboard.h"
#include "game/input/common.h"
#include "game/lara.h"
#include "game/lua.h"
#include "game/random.h"
#include "game/shell.h"
#include "game/shell/events.h"
#include "memory.h"
#include "vector.h"

#include <ctype.h>
#include <string.h>

#define M_DEBUG 0

typedef struct {
    VECTOR *raw_args;
} M_PARSE_CTX;

typedef struct {
    MYFILE *file;
    int32_t frame_idx;
    int32_t next_frame_idx;
    VECTOR *queue;
} M_PRIV;

static M_PRIV m_Priv = {};

typedef bool (*M_EVENT_HANDLER)(const char *token);
typedef bool (*M_HEADER_HANDLER)(const char *line, M_PARSE_CTX *ctx);

// Event parsers
static bool M_ParseQuitEvent(const char *event_str);
static bool M_ParseKeyDownEvent(const char *event_str);
static bool M_ParseKeyUpEvent(const char *event_str);
static bool M_ParseTextInputEvent(const char *event_str);
static bool M_ParseCommandEvent(const char *event_str);
static bool M_ParseLuaEvent(const char *event_str);
static bool M_ParseAssertEvent(const char *event_str);

// Header parsers
static bool M_ParseSeedControl(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseSeedDraw(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseBindKeyboard(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseBindController(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseBindController(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseArgs(const char *line, M_PARSE_CTX *ctx);
static bool M_ParseConfig(const char *line, M_PARSE_CTX *ctx);

static bool M_ParseEvent(const char *event_str);
static void M_ReadHeaders(M_PRIV *p, M_PARSE_CTX *ctx);
static void M_ClearEventQueue(M_PRIV *p);
static void M_StripInlineComment(char *const line);
static void M_ReadQueue(M_PRIV *p);
static void M_RunQueue(M_PRIV *p);

static const M_HEADER_HANDLER m_HeaderHandlers[] = {
    M_ParseSeedControl,
    M_ParseSeedDraw,
    M_ParseBindKeyboard,
    M_ParseBindController,
    M_ParseArgs,
    M_ParseConfig,
    nullptr,
};

static const M_EVENT_HANDLER m_EventHandlers[] = {
    M_ParseQuitEvent,
    M_ParseKeyDownEvent,
    M_ParseKeyUpEvent,
    M_ParseTextInputEvent,
    M_ParseCommandEvent,
    M_ParseLuaEvent,
#if M_DEBUG
    M_ParseAssertEvent,
#endif
    nullptr,
};

static bool M_ParseQuitEvent(const char *const event_str)
{
    if (strcmp(event_str, "quit") != 0) {
        return false;
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
    return M_ParseKeyEvent(event_str, SDL_KEYDOWN, "keydown");
}

static bool M_ParseKeyUpEvent(const char *event_str)
{
    return M_ParseKeyEvent(event_str, SDL_KEYUP, "keyup");
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

static bool M_ParseCommandEvent(const char *const event_str)
{
    if (strncmp(event_str, "cmd ", 4) != 0) {
        return false;
    }

    const char *const start = strchr(event_str + 4, '"');
    const char *const end = start ? strrchr(start + 1, '"') : nullptr;
    if (start == nullptr || end == nullptr || end <= start + 1) {
        LOG_WARNING("Malformed cmd instruction: %s", event_str);
        return false;
    }
    const size_t len = (size_t)(end - (start + 1));
    char *const cmd_str = Memory_DupStr(start + 1);
    if (strlen(cmd_str) > len) {
        cmd_str[len] = '\0';
    }
    Console_Eval(cmd_str);
    return true;
}

static bool M_ParseLuaEvent(const char *const event_str)
{
    M_PRIV *const p = &m_Priv;
    if (strncmp(event_str, "lua ", 4) != 0) {
        return false;
    }

    LUA_RESULT eval_result = Lua_Eval(event_str + 4);
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
    Lua_FreeResult(&eval_result);
    return true;
}

static bool M_ParseAssertEvent(const char *const event_str)
{
    const ITEM *const lara_item = Lara_GetItem();
    const GAME_OBJECT_ID obj_id = Lara_GetAnimationObject();
    const ITEM *const vehicle_item = Lara_Vehicle_GetItem();
    int32_t x, y, z;
    if (sscanf(event_str, "assert lara.pos=%d,%d,%d", &x, &y, &z) == 3) {
        ASSERT(lara_item->pos.x == x);
        ASSERT(lara_item->pos.y == y);
        ASSERT(lara_item->pos.z == z);
        return true;
    } else if (sscanf(event_str, "assert lara.rot=%d,%d,%d", &x, &y, &z) == 3) {
        ASSERT(lara_item->rot.x == x);
        ASSERT(lara_item->rot.y == y);
        ASSERT(lara_item->rot.z == z);
        return true;
    } else if (
        sscanf(event_str, "assert lara.anim=%d,%d,%d", &x, &y, &z) == 3) {
        ASSERT(x == obj_id);
        ASSERT(y == Item_GetRelativeObjAnim(lara_item, obj_id));
        ASSERT(z == Item_GetRelativeFrame(lara_item));
        return true;
    } else if (sscanf(event_str, "assert lara.speed=%d,%d", &x, &y) == 2) {
        ASSERT(
            x == (vehicle_item != nullptr ? vehicle_item : lara_item)->speed);
        ASSERT(
            y
            == (vehicle_item != nullptr ? vehicle_item : lara_item)
                   ->fall_speed);
        return true;
    }
    return false;
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

static void M_ClearQueue(M_PRIV *const p)
{
    if (p->queue != nullptr) {
        for (int32_t i = 0; i < p->queue->count; i++) {
            char *event_str = *(char **)Vector_Get(p->queue, i);
            Memory_Free(event_str);
        }
        Vector_Clear(p->queue);
    }
}

static void M_StripInlineComment(char *const line)
{
    bool in_quote = false;
    char *p;
    for (p = line; *p != '\0'; ++p) {
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

static void M_ReadQueue(M_PRIV *const p)
{
    M_ClearQueue(p);

    while (true) {
        char *const line = (char *)File_ReadLine(p->file);
        if (line == nullptr) {
            break;
        }
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        if (line[strlen(line) - 1] == '\n') {
            line[strlen(line) - 1] = '\0';
        }
        M_StripInlineComment(line);

        // Expect a frame marker: @+<delta>:
        char *const colon = strchr(line, ':');
        if (colon == nullptr) {
            LOG_ERROR("Malformed input line: %s", line);
            continue;
        }
        int32_t delta = 0;
        {
            char *marker = line;
            if (*marker == '@') {
                marker++;
            }
            if (*marker != '+' || sscanf(marker + 1, "%d", &delta) != 1) {
                LOG_ERROR("Malformed input line: %s", line);
                continue;
            }
        }
        p->next_frame_idx = p->frame_idx + delta;

        // First event on this line (after colon)
        char *p_event = colon + 1;
        while (*p_event == ' ' || *p_event == '\t') {
            p_event++;
        }
        if (*p_event != '\0') {
            char *const event_str = Memory_DupStr(p_event);
            Vector_Add(p->queue, &event_str);
        }

        // Continuation lines for this frame
        while (true) {
            char *const cont = (char *)File_ReadLine(p->file);
            if (cont == nullptr) {
                break;
            }
            if (cont[0] == '#' || cont[0] == '\n') {
                continue;
            }
            size_t len = strlen(cont);
            if (len > 0 && cont[len - 1] == '\n') {
                cont[len - 1] = '\0';
            }
            M_StripInlineComment(cont);
            if (cont[0] != ' ' && cont[0] != '\t') {
                File_Skip(p->file, -strlen(cont));
                break;
            }
            char *p_cont = cont;
            while (*p_cont == ' ' || *p_cont == '\t') {
                p_cont++;
            }
            if (*p_cont != '\0') {
                char *const event_str = Memory_DupStr(p_cont);
                Vector_Add(p->queue, &event_str);
            }
        }
        return;
    }

    // no more frames
    p->next_frame_idx = -1;
}

static void M_RunQueue(M_PRIV *const p)
{
    for (int32_t i = 0; i < p->queue->count; i++) {
        const char *const event_str = *(char **)Vector_Get(p->queue, i);
        M_ParseEvent(event_str);
    }
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
    if (start == nullptr || end == nullptr || end <= start + 1) {
        LOG_WARNING("Malformed bind keyboard instruction: %s", line);
        return false;
    }
    const size_t slen = end - (start + 1);
    const char *desc = String_FormatStatic("%.*s", slen, start + 1);
    SDL_Scancode sc;
    SDL_Keymod mod;
    if (!Input_ParseKeyDesc(desc, &sc, &mod)) {
        return false;
    }
    JSON_OBJECT *const bind = JSON_ObjectNew();
    JSON_ObjectAppendInt(bind, "scancode", sc);
    JSON_ObjectAppendInt(bind, "mod", mod);
    g_Input_Keyboard.assign_from_json_object(
        g_Config.input.keyboard_layout, role, bind);
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
            g_Config.input.controller_layout, role, bind);
        JSON_ObjectFree(bind);
        return true;
    }
    return false;
}

static bool M_ParseArgs(const char *const line, M_PARSE_CTX *const ctx)
{
    if (strncmp(line, "args", 4) != 0) {
        return false;
    }

    ctx->raw_args = Vector_Create(sizeof(const char *));
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
        Vector_Add(ctx->raw_args, &arg);
        if (*p == '"') {
            p++;
        }
    }
    return true;
}

static bool M_ParseConfig(const char *const line, M_PARSE_CTX *const ctx)
{
    char keybuf[64];
    char valbuf[128];
    if (sscanf(line, "config %63s %127s", keybuf, valbuf) == 2) {
        // Strip surrounding quotes from the value, if present
        size_t vlen = strlen(valbuf);
        if (vlen >= 2 && valbuf[0] == '"' && valbuf[vlen - 1] == '"') {
            valbuf[vlen - 1] = '\0';
            memmove(valbuf, valbuf + 1, vlen - 1);
        }
        const CONFIG_OPTION *opt = Config_GetOptionByPath(keybuf);
        if (opt) {
            Config_SetOptionValueFromString(opt, valbuf);
        } else {
            LOG_WARNING("Unknown option: %s", keybuf);
        }
        return true;
    }
    return false;
}

static void M_ReadHeaders(M_PRIV *const p, M_PARSE_CTX *const ctx)
{
    while (true) {
        char *const line = (char *)File_ReadLine(p->file);
        if (line == nullptr) {
            break;
        }
        M_StripInlineComment(line);

        // Skip comments and blank lines
        if (line[0] == '\n') {
            continue;
        }

        // Stop at first replay frame marker (@+<delta>:), rewind to allow its
        // parsing
        {
            int32_t rel = 0;
            if (sscanf(line, "@+%d:", &rel) == 1) {
                File_Skip(p->file, -strlen(line));
                break;
            }
        }

        // Dispatch based on matching prefix
        bool handled = false;
        for (int32_t i = 0; m_HeaderHandlers[i] != nullptr; i++) {
            if (m_HeaderHandlers[i](line, ctx)) {
                handled = true;
                break;
            }
        }

        // Unknown #comment? Just ignore
        if (!handled) {
            LOG_WARNING("Unknown line: %s", line);
        }
    }
}

SHELL_ARGS *TestReplay_Open(const char *path)
{
    M_PRIV *const p = &m_Priv;
    p->file = File_Open(path, FILE_OPEN_READ);
    if (p->file == nullptr) {
        Shell_ExitSystemFmt("Cannot open replay file '%s'", path);
        return nullptr;
    }

    // Read header for initialization stuff
    M_PARSE_CTX ctx = {};
    M_ReadHeaders(p, &ctx);
    SHELL_ARGS *const new_args = Shell_ParseArgs(ctx.raw_args);

    p->next_frame_idx = -1;
    p->queue = Vector_Create(sizeof(char *));
    M_ReadQueue(p);
    LOG_INFO("Starting playback");
    return new_args;
}

void TestReplay_Close(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->file != nullptr) {
        File_Close(p->file);
        p->file = nullptr;
    }
    if (p->queue != nullptr) {
        M_ClearQueue(p);
        Vector_Free(p->queue);
        p->queue = nullptr;
    }
}

bool TestReplay_IsOpened(void)
{
    M_PRIV *const p = &m_Priv;
    return p->file != nullptr;
}

void TestReplay_RunFrame(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->next_frame_idx == p->frame_idx) {
        M_RunQueue(p);
        M_ReadQueue(p);
    }
    p->frame_idx++;
}
