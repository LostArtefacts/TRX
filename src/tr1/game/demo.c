#include "game/demo.h"

#include "game/camera.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/interpolation.h"
#include "game/item_actions.h"
#include "game/lara/cheat.h"
#include "game/lara/common.h"
#include "game/lara/hair.h"
#include "game/level.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/phase.h"
#include "game/random.h"
#include "game/room.h"
#include "game/savegame.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/log.h>

#define MODIFY_CONFIG()                                                        \
    PROCESS_CONFIG(gameplay.enable_enhanced_look, false);                      \
    PROCESS_CONFIG(gameplay.enable_tr2_jumping, false);                        \
    PROCESS_CONFIG(gameplay.enable_tr2_swimming, false);                       \
    PROCESS_CONFIG(gameplay.enable_tr2_swim_cancel, false);                    \
    PROCESS_CONFIG(gameplay.enable_wading, false);                             \
    PROCESS_CONFIG(gameplay.target_mode, TLM_FULL);                            \
    PROCESS_CONFIG(gameplay.fix_bear_ai, false);

typedef struct {
    const uint32_t *demo_ptr;
    const GF_LEVEL *level;
    CONFIG old_config;
    TEXTSTRING *text;
} M_PRIV;

static int32_t m_LastDemoNum = 0;
static M_PRIV m_Priv;

static void M_PrepareConfig(M_PRIV *const p);
static void M_RestoreConfig(M_PRIV *const p);
static bool M_ProcessInput(M_PRIV *const p);

static void M_PrepareConfig(M_PRIV *const p)
{
    // Changing certains settings affects negatively the original game demo
    // data, so temporarily turn off all relevant enhancements.
    p->old_config = g_Config;
#undef PROCESS_CONFIG
#define PROCESS_CONFIG(var, value) g_Config.var = value;
    MODIFY_CONFIG();
}

static void M_RestoreConfig(M_PRIV *const p)
{
#undef PROCESS_CONFIG
#define PROCESS_CONFIG(var, value) g_Config.var = p->old_config.var;
    MODIFY_CONFIG();
}

static bool M_ProcessInput(M_PRIV *const p)
{
    union {
        uint32_t any;
        struct {
            // clang-format off
            uint32_t forward:      1;
            uint32_t back:         1;
            uint32_t left:         1;
            uint32_t right:        1;
            uint32_t jump:         1;
            uint32_t draw:         1;
            uint32_t action:       1;
            uint32_t slow:         1;
            uint32_t option:       1;
            uint32_t look:         1;
            uint32_t step_left:    1;
            uint32_t step_right:   1;
            uint32_t roll:         1;
            uint32_t _pad:         7;
            uint32_t menu_confirm: 1;
            uint32_t menu_back:    1;
            uint32_t save:         1;
            uint32_t load:         1;
            // clang-format on
        };
    } demo_input = { .any = *p->demo_ptr };

    if ((int32_t)demo_input.any == -1) {
        return false;
    }

    // Translate demo inputs (that use TombATI key values) to TR1X inputs.
    g_Input = (INPUT_STATE) {
        // clang-format off
        .forward      = demo_input.forward,
        .back         = demo_input.back,
        .left         = demo_input.left,
        .right        = demo_input.right,
        .jump         = demo_input.jump,
        .draw         = demo_input.draw,
        .action       = demo_input.action,
        .slow         = demo_input.slow,
        .option       = demo_input.option,
        .look         = demo_input.look,
        .step_left    = demo_input.step_left,
        .step_right   = demo_input.step_right,
        .roll         = demo_input.roll,
        .menu_confirm = demo_input.menu_confirm,
        .menu_back    = demo_input.menu_back,
        .save         = demo_input.save,
        .load         = demo_input.load,
        // clang-format on
    };

    p->demo_ptr++;
    return true;
}

bool Demo_Start(const int32_t level_num)
{
    M_PRIV *const p = &m_Priv;
    p->level = GF_GetLevel(GFLT_DEMOS, level_num);
    ASSERT(p->level != nullptr);
    ASSERT(GF_GetCurrentLevel() == p->level);

    M_PrepareConfig(p);

    // Remember old inputs in case the demo was forcefully started with some
    // keys pressed. In that case, it should only be stopped if the user
    // presses some other key.
    Input_Update();

    Interpolation_Remember();

    const uint32_t *const data = Demo_GetData();
    if (data == nullptr) {
        LOG_ERROR("Level '%s' has no demo data", p->level->path);
        return false;
    }

    g_OverlayFlag = 1;
    Camera_Initialise();
    p->demo_ptr = data;

    ITEM *const lara_item = Lara_GetItem();
    lara_item->pos.x = *p->demo_ptr++;
    lara_item->pos.y = *p->demo_ptr++;
    lara_item->pos.z = *p->demo_ptr++;
    lara_item->rot.x = *p->demo_ptr++;
    lara_item->rot.y = *p->demo_ptr++;
    lara_item->rot.z = *p->demo_ptr++;
    int16_t room_num = *p->demo_ptr++;

    if (lara_item->room_num != room_num) {
        Item_NewRoom(g_Lara.item_num, room_num);
    }

    const SECTOR *const sector = Room_GetSector(
        lara_item->pos.x, lara_item->pos.y, lara_item->pos.z, &room_num);
    lara_item->floor = Room_GetHeight(
        sector, lara_item->pos.x, lara_item->pos.y, lara_item->pos.z);

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // LaraGun() expects request_gun_type to be set only when it
    // really is needed, not at all times.
    // https://github.com/LostArtefacts/TRX/issues/36
    g_Lara.request_gun_type = LGT_UNARMED;

    p->text = Text_Create(0, -16, GS(MISC_DEMO_MODE));
    Text_Flash(p->text, true, 20);
    Text_AlignBottom(p->text, true);
    Text_CentreH(p->text, true);
    g_GameInfo.showing_demo = true;
    return true;
}

void Demo_End(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Text_Remove(p->text);
    p->text = nullptr;
    g_GameInfo.showing_demo = false;
}

void Demo_Pause(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Text_Hide(p->text, true);
    g_GameInfo.showing_demo = false;
}

void Demo_Unpause(void)
{
    M_PRIV *const p = &m_Priv;
    M_PrepareConfig(p);
    Text_Hide(p->text, false);
    g_GameInfo.showing_demo = true;
}

int32_t Demo_ChooseLevel(const int32_t demo_num)
{
    M_PRIV *const p = &m_Priv;
    const int32_t demo_count = GF_GetLevelTable(GFLT_DEMOS)->count;
    if (demo_count <= 0) {
        return -1;
    } else if (demo_num < 0 || demo_num >= demo_count) {
        return (m_LastDemoNum++) % demo_count;
    } else {
        return demo_num;
    }
}

GF_COMMAND Demo_Control(void)
{
    Interpolation_Remember();
    M_PRIV *const p = &m_Priv;

    Input_Update();
    Shell_ProcessInput();

    if (g_InputDB.pause) {
        PHASE *const subphase = Phase_Pause_Create();
        const GF_COMMAND gf_cmd = PhaseExecutor_Run(subphase);
        Phase_Pause_Destroy(subphase);
        return gf_cmd;
    } else if (g_InputDB.toggle_photo_mode) {
        PHASE *const subphase = Phase_PhotoMode_Create();
        const GF_COMMAND gf_cmd = PhaseExecutor_Run(subphase);
        Phase_PhotoMode_Destroy(subphase);
        return gf_cmd;
    }

    if (g_LevelComplete || g_InputDB.menu_confirm || g_InputDB.menu_back) {
        return (GF_COMMAND) {
            .action = GF_EXIT_TO_TITLE,
            .param = p->level->num,
        };
    }
    g_Input.any = 0;
    g_InputDB.any = 0;

    if (!M_ProcessInput(p)) {
        return (GF_COMMAND) {
            .action = GF_EXIT_TO_TITLE,
            .param = p->level->num,
        };
    }
    Lara_Cheat_Control();

    Game_ProcessInput();

    Output_ResetDynamicLights();

    Item_Control();
    Effect_Control();

    Lara_Control();
    Lara_Hair_Control();

    Camera_Update();
    Sound_ResetAmbient();
    ItemAction_RunActive();
    Sound_UpdateEffects();
    Overlay_BarHealthTimerTick();
    Output_AnimateTextures(1);

    return (GF_COMMAND) { .action = GF_NOOP };
}

void Demo_StopFlashing(void)
{
    M_PRIV *const p = &m_Priv;
    Text_Flash(p->text, false, 0);
}
