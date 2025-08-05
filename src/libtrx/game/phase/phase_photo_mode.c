#include "game/phase/phase_photo_mode.h"

#include "config.h"
#include "game/camera.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/lara/pose.h"
#include "game/math.h"
#include "game/matrix.h"
#include "game/music.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/phase/executor.h"
#include "game/photo_mode.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/ui.h"
#include "memory.h"

typedef struct {
    bool taking_screenshot;
} M_PRIV;

static PHASE_CONTROL M_Start(PHASE *phase);
static void M_End(PHASE *phase);
static PHASE_CONTROL M_Control(PHASE *phase);
static void M_Draw(PHASE *phase);

static PHASE_CONTROL M_Start(PHASE *phase)
{
    PhotoMode_Start();
    return (PHASE_CONTROL) { .action = PHASE_ACTION_CONTINUE };
}

static void M_End(PHASE *const phase)
{
    PhotoMode_End();
}

static PHASE_CONTROL M_Control(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Input_Update();
    Shell_ProcessInput();

    if (g_InputDB.toggle_photo_mode || g_InputDB.menu_back) {
        return (PHASE_CONTROL) {
            .action = PHASE_ACTION_END,
            .gf_cmd = { .action = GF_NOOP },
        };
    } else if (g_InputDB.action) {
        p->taking_screenshot = true;
        Screenshot_Make(g_Config.rendering.screenshot_format);
        Sound_Effect(SFX_MENU_LARA_HOME, nullptr, SPM_ALWAYS);
    }
    return PhotoMode_Control();
}

static void M_Draw(PHASE *const phase)
{
    M_PRIV *const p = phase->priv;
    Game_Draw(false);

    if (p->taking_screenshot) {
        p->taking_screenshot = false;
    } else {
        UI_PhotoMode(PhotoMode_GetCurrentMode());
    }
}

PHASE *Phase_PhotoMode_Create(void)
{
    PHASE *const phase = Memory_Alloc(sizeof(PHASE));
    phase->priv = Memory_Alloc(sizeof(M_PRIV));
    phase->start = M_Start;
    phase->end = M_End;
    phase->control = M_Control;
    phase->draw = M_Draw;
    return phase;
}

void Phase_PhotoMode_Destroy(PHASE *phase)
{
    Memory_Free(phase->priv);
    Memory_Free(phase);
}
