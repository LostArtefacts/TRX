#include "game/test_recorder.h"

#include "config.h"
#include "debug.h"
#include "filesystem.h"
#include "game/console/common.h"
#include "game/input/backends/controller.h"
#include "game/input/backends/keyboard.h"
#include "game/lara.h"
#include "game/random.h"

#define M_DEBUG 0
#define M_MAX_EVENTS 64 // Maximum SDL or custom events per frame

typedef struct {
    MYFILE *file;
    int32_t frame_idx;
    SDL_Event queue[M_MAX_EVENTS];
    int32_t queue_size;
} M_PRIV;

static M_PRIV m_Priv = {};

static const char *M_DumpEvent(const SDL_Event *event);
static void M_DumpQueue(M_PRIV *p);

static const char *M_DumpEvent(const SDL_Event *const event)
{
    switch (event->type) {
    case SDL_KEYDOWN:
        return String_FormatStatic(
            "keydown %d %hu", event->key.keysym.scancode,
            event->key.keysym.mod);
    case SDL_TEXTINPUT:
        return String_FormatStatic("text-input \"%s\"", event->text.text);
    case SDL_KEYUP:
        return String_FormatStatic(
            "keyup %d %hu", event->key.keysym.scancode, event->key.keysym.mod);
    case SDL_QUIT:
        return String_FormatStatic("quit");
    }
    return nullptr;
}

static void M_DumpQueue(M_PRIV *const p)
{
#if !M_DEBUG
    if (p->queue_size == 0) {
        return;
    }
#endif
    File_WriteString(p->file, "frame %d:", p->frame_idx);
    for (int32_t i = 0; i < p->queue_size; i++) {
        const SDL_Event *const event = &p->queue[i];
        const char *const event_str = M_DumpEvent(event);
        if (event_str == nullptr) {
            continue;
        }
        File_WriteString(p->file, " ");
        File_WriteString(p->file, event_str);
        if (i < p->queue_size - 1) {
            File_WriteString(p->file, ";");
        }
    }
#if M_DEBUG
    const ITEM *const lara_item = Lara_GetItem();
    const GAME_OBJECT_ID obj_id = Lara_GetAnimationObject();
    const ITEM *const vehicle_item = Lara_Vehicle_GetItem();
    if (lara_item != nullptr) {
        File_WriteString(
            p->file, "; assert lara.pos=%d,%d,%d", lara_item->pos.x,
            lara_item->pos.y, lara_item->pos.z);
        File_WriteString(
            p->file, "; assert lara.rot=%d,%d,%d", lara_item->rot.x,
            lara_item->rot.y, lara_item->rot.z);
        File_WriteString(
            p->file, "; assert lara.anim=%d,%d,%d", obj_id,
            Item_GetRelativeObjAnim(lara_item, obj_id),
            Item_GetRelativeFrame(lara_item));
        File_WriteString(
            p->file, "; assert lara.speed=%d,%d",
            (vehicle_item != nullptr ? vehicle_item : lara_item)->speed,
            (vehicle_item != nullptr ? vehicle_item : lara_item)->fall_speed);
    }
#endif
    File_WriteString(p->file, "\n");
}

void TestRecorder_Open(const char *path, VECTOR *const original_args)
{
    M_PRIV *const p = &m_Priv;
    p->file = File_Open(path, FILE_OPEN_WRITE);
    if (p->file == nullptr) {
        LOG_ERROR("Cannot open record file '%s'", path);
        return;
    }
    File_WriteString(p->file, "seed_control %d\n", Random_GetControlSeed());
    File_WriteString(p->file, "seed_draw %d\n", Random_GetDrawSeed());

    // Record original arguments passed to the game
    if (original_args->count > 0) {
        // Skip tracking irrelevant arguments.
        VECTOR *const filtered_args = Vector_Create(sizeof(char *));
        for (int32_t i = 0; i < original_args->count; i++) {
            const char *const arg = *(char **)Vector_Get(original_args, i);
            if (!strcmp(arg, "--test-record") || !strcmp(arg, "--test-replay")
                || !strcmp(arg, "--test-play")
                || !strcmp(arg, "--headless-fps")) {
                i++; // Also skip the path argument.
                continue;
            }
            if (!strcmp(arg, "--debug-render-performance")) {
                continue;
            }
            Vector_Add(filtered_args, &arg);
        }

        if (filtered_args->count > 0) {
            File_WriteString(p->file, "args");
            for (int32_t i = 0; i < filtered_args->count; i++) {
                const char *const arg = *(char **)Vector_Get(filtered_args, i);
                File_WriteString(p->file, " \"%s\"", arg);
            }
            File_WriteString(p->file, "\n");
        }
        Vector_Free(filtered_args);
    }

    // Record any non-default config options for later replay
    for (const CONFIG_OPTION *opt = Config_GetOptionMap(); opt->name != nullptr;
         opt++) {
        if (!Config_IsOptionAtDefault(opt->target)) {
            File_WriteString(
                p->file, "config %s %s\n", opt->name,
                Config_GetOptionValueAsString(opt));
        }
    }

    // Record any non-default key/controller bindings for later replay.
    // Keyboard binds
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        JSON_OBJECT *bind = JSON_ObjectNew();
        if (g_Input_Keyboard.assign_to_json_object(
                g_Config.input.keyboard_layout, role, bind)) {
            const int32_t sc = JSON_ObjectGetInt(bind, "scancode", -1);
            File_WriteString(p->file, "bind keyboard %d %d\n", role, sc);
        }
        JSON_ObjectFree(bind);
    }
    // Controller binds
    for (INPUT_ROLE role = 0; role < INPUT_ROLE_NUMBER_OF; role++) {
        JSON_OBJECT *bind = JSON_ObjectNew();
        if (g_Input_Controller.assign_to_json_object(
                g_Config.input.controller_layout, role, bind)) {
            const int32_t bt = JSON_ObjectGetInt(bind, "button_type", 0);
            const int32_t b = JSON_ObjectGetInt(bind, "bind", 0);
            const int32_t ad = JSON_ObjectGetInt(bind, "axis_dir", 0);
            File_WriteString(
                p->file, "bind controller %d %d %d %d\n", role, bt, b, ad);
        }
        JSON_ObjectFree(bind);
    }
    File_WriteString(p->file, "\n");

    LOG_INFO("Starting recording");
}

bool TestRecorder_IsOpened(void)
{
    M_PRIV *const p = &m_Priv;
    return p->file != nullptr;
}

void TestRecorder_Close(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->file != nullptr) {
        File_Close(p->file);
        p->file = nullptr;
    }
}

void TestRecorder_BeginFrame(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->file != nullptr) {
        p->queue_size = 0;
    }
}

void TestRecorder_EndFrame(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->file != nullptr) {
        M_DumpQueue(p);
    }
    p->frame_idx++;
}

void TestRecorder_RecordEvent(const SDL_Event *const event)
{
    M_PRIV *const p = &m_Priv;
    if (p->file == nullptr) {
        return;
    }

    // Only record eligible events
    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP
        && event->type != SDL_QUIT && event->type != SDL_TEXTINPUT) {
        return;
    }
    if (event->type == SDL_KEYDOWN && event->key.repeat) {
        return;
    }
    if (event->type == SDL_TEXTINPUT && !Console_IsOpened()) {
        return;
    }

    if (p->queue_size < M_MAX_EVENTS) {
        p->queue[p->queue_size++] = *event;
    }
}
