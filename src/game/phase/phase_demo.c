#include "game/phase/phase_demo.h"

#include "config.h"
#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lara/lara_cheat.h"
#include "game/lara/lara_hair.h"
#include "game/level.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/phase/phase.h"
#include "game/random.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum STATE {
    STATE_RUN,
    STATE_FADE_OUT,
    STATE_INVALID,
} STATE;

static bool m_OldEnhancedLook;
static bool m_OldTR2Jumping;
static TARGET_LOCK_MODE m_OldTargetMode;
static RESUME_INFO m_OldResumeInfo;
static TEXTSTRING *m_DemoModeText = NULL;
static STATE m_State = STATE_RUN;
static GAMEFLOW_OPTION m_Result = GF_EXIT_TO_TITLE;

static int32_t m_DemoLevel = -1;
static uint32_t *m_DemoPtr = NULL;

static bool Phase_Demo_ProcessInput(void);
static int32_t Phase_Demo_ChooseLevel(void);

static void Phase_Demo_Start(void *arg);
static void Phase_Demo_End(void);
static GAMEFLOW_OPTION Phase_Demo_Run(int32_t nframes);
static GAMEFLOW_OPTION Phase_Demo_FadeOut(void);
static GAMEFLOW_OPTION Phase_Demo_Control(int32_t nframes);
static void Phase_Demo_Draw(void);

static bool Phase_Demo_ProcessInput(void)
{
    if (m_DemoPtr >= &g_DemoData[DEMO_COUNT_MAX] || (int)*m_DemoPtr == -1) {
        return false;
    }

    // Translate demo inputs (that use TombATI key values) to TR1X inputs.
    g_Input = (INPUT_STATE) {
        0,
        .forward = (bool)(*m_DemoPtr & (1 << 0)),
        .back = (bool)(*m_DemoPtr & (1 << 1)),
        .left = (bool)(*m_DemoPtr & (1 << 2)),
        .right = (bool)(*m_DemoPtr & (1 << 3)),
        .jump = (bool)(*m_DemoPtr & (1 << 4)),
        .draw = (bool)(*m_DemoPtr & (1 << 5)),
        .action = (bool)(*m_DemoPtr & (1 << 6)),
        .slow = (bool)(*m_DemoPtr & (1 << 7)),
        .option = (bool)(*m_DemoPtr & (1 << 8)),
        .look = (bool)(*m_DemoPtr & (1 << 9)),
        .step_left = (bool)(*m_DemoPtr & (1 << 10)),
        .step_right = (bool)(*m_DemoPtr & (1 << 11)),
        .roll = (bool)(*m_DemoPtr & (1 << 12)),
        .menu_confirm = (bool)(*m_DemoPtr & (1 << 20)),
        .menu_back = (bool)(*m_DemoPtr & (1 << 21)),
        .save = (bool)(*m_DemoPtr & (1 << 22)),
        .load = (bool)(*m_DemoPtr & (1 << 23)),
    };

    m_DemoPtr++;
    return true;
}

static int32_t Phase_Demo_ChooseLevel(void)
{
    bool any_demos = false;
    for (int i = g_GameFlow.first_level_num; i < g_GameFlow.last_level_num;
         i++) {
        if (g_GameFlow.levels[i].demo) {
            any_demos = true;
        }
    }
    if (!any_demos) {
        return -1;
    }

    int16_t level_num = m_DemoLevel;
    do {
        level_num++;
        if (level_num > g_GameFlow.last_level_num) {
            level_num = g_GameFlow.first_level_num;
        }
    } while (!g_GameFlow.levels[level_num].demo);
    return level_num;
}

static void Phase_Demo_Start(void *arg)
{
    m_DemoLevel = Phase_Demo_ChooseLevel();

    if (m_DemoLevel == -1) {
        m_State = STATE_INVALID;
        return;
    }

    m_State = STATE_RUN;

    Interpolation_Enable();
    Interpolation_Remember();
    Output_FadeReset();

    RESUME_INFO *resume_info = &g_GameInfo.current[m_DemoLevel];
    m_OldResumeInfo = *resume_info;
    resume_info->flags.available = 1;
    resume_info->flags.got_pistols = 1;
    resume_info->pistol_ammo = 1000;
    resume_info->gun_status = LGS_ARMLESS;
    resume_info->gun_type = LGT_PISTOLS;
    resume_info->lara_hitpoints = LARA_HITPOINTS;

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // changing the controls affects negatively the original game demo data,
    // so temporarily turn off all the TR1X enhancements
    m_OldEnhancedLook = g_Config.enable_enhanced_look;
    m_OldTR2Jumping = g_Config.enable_tr2_jumping;
    m_OldTargetMode = g_Config.target_mode;
    g_Config.enable_enhanced_look = false;
    g_Config.enable_tr2_jumping = false;
    g_Config.target_mode = TLM_FULL;

    m_DemoModeText = Text_Create(0, -16, g_GameFlow.strings[GS_MISC_DEMO_MODE]);
    Text_Flash(m_DemoModeText, 1, 20);
    Text_AlignBottom(m_DemoModeText, 1);
    Text_CentreH(m_DemoModeText, 1);

    if (!Level_Initialise(m_DemoLevel)) {
        Shell_ExitSystem("Unable to initialize level");
    }

    g_OverlayFlag = 1;
    Camera_Initialise();

    m_DemoPtr = g_DemoData;

    ITEM_INFO *item = g_LaraItem;
    item->pos.x = *m_DemoPtr++;
    item->pos.y = *m_DemoPtr++;
    item->pos.z = *m_DemoPtr++;
    item->rot.x = *m_DemoPtr++;
    item->rot.y = *m_DemoPtr++;
    item->rot.z = *m_DemoPtr++;
    int16_t room_num = *m_DemoPtr++;

    if (item->room_number != room_num) {
        Item_NewRoom(g_Lara.item_number, room_num);
    }

    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // LaraGun() expects request_gun_type to be set only when it
    // really is needed, not at all times.
    // https://github.com/LostArtefacts/TR1X/issues/36
    g_Lara.request_gun_type = LGT_UNARMED;
    g_GameInfo.current_level_type = GFL_DEMO;
}

static void Phase_Demo_End(void)
{
    if (m_DemoLevel == -1) {
        return;
    }

    Interpolation_Disable();
    Text_Remove(m_DemoModeText);

    g_GameInfo.current[m_DemoLevel] = m_OldResumeInfo;

    g_Config.target_mode = m_OldTargetMode;
    g_Config.enable_enhanced_look = m_OldEnhancedLook;
    g_Config.enable_tr2_jumping = m_OldTR2Jumping;
}

static GAMEFLOW_OPTION Phase_Demo_Run(int32_t nframes)
{
    Interpolation_Remember();
    CLAMPG(nframes, MAX_FRAMES);

    for (int32_t i = 0; i < nframes; i++) {
        Lara_CheckCheatMode();
        if (g_LevelComplete) {
            m_State = STATE_FADE_OUT;
            return GF_PHASE_CONTINUE;
        }

        Input_Update();
        Shell_ProcessInput();
        Game_ProcessInput();

        if (g_Input.any || !Phase_Demo_ProcessInput()) {
            m_State = STATE_FADE_OUT;
            return GF_PHASE_CONTINUE;
        }

        Item_Control();
        Effect_Control();

        Lara_Control();
        Lara_Hair_Control();

        Camera_Update();
        Sound_ResetAmbient();
        Effect_RunActiveFlipEffect();
        Sound_UpdateEffects();
        g_GameInfo.current[g_CurrentLevel].stats.timer++;
        Overlay_BarHealthTimerTick();
    }

    return GF_PHASE_CONTINUE;
}

static GAMEFLOW_OPTION Phase_Demo_FadeOut(void)
{
    Text_Flash(m_DemoModeText, 0, 0);
    Input_Update();
    Output_FadeToBlack(true);
    if (g_InputDB.menu_confirm || g_InputDB.menu_back
        || !Output_FadeIsAnimating()) {
        Output_FadeResetToBlack();
        return GF_EXIT_TO_TITLE;
    }
    return GF_PHASE_CONTINUE;
}

static GAMEFLOW_OPTION Phase_Demo_Control(int32_t nframes)
{
    switch (m_State) {
    case STATE_INVALID:
        return GF_EXIT_TO_TITLE;

    case STATE_RUN:
        return Phase_Demo_Run(nframes);

    case STATE_FADE_OUT:
        return Phase_Demo_FadeOut();
    }

    assert(false);
    return GF_PHASE_BREAK;
}

static void Phase_Demo_Draw(void)
{
    Game_DrawScene(true);
    Output_AnimateTextures();
    Output_AnimateFades();
    Text_Draw();
}

PHASER g_DemoPhaser = {
    .start = Phase_Demo_Start,
    .end = Phase_Demo_End,
    .control = Phase_Demo_Control,
    .draw = Phase_Demo_Draw,
    .wait = NULL,
};
