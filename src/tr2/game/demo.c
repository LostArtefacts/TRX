#include "game/demo.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/savegame.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/input.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/log.h>

#define L_MODIFY_CONFIG()                                                      \
    X_PROCESS_CONFIG(gameplay.harpoon_recoil, 4);                              \
    X_PROCESS_CONFIG(gameplay.start_lara_hitpoints, LARA_MAX_HITPOINTS);       \
    X_PROCESS_CONFIG(gameplay.disable_healing_between_levels, false);          \
    X_PROCESS_CONFIG(gameplay.wall_glitch_mode, WALL_GLITCH_TR2);              \
    X_PROCESS_CONFIG(gameplay.look_mode, LOOK_MODE_ENHANCED);                  \
    X_PROCESS_CONFIG(gameplay.enable_tr2_swimming, true);                      \
    X_PROCESS_CONFIG(gameplay.target_mode, TLM_FULL);                          \
    X_PROCESS_CONFIG(gameplay.enable_target_change, false);                    \
    X_PROCESS_CONFIG(input.quick_guns_mode, QUICK_GUNS_DRAW_ONLY);             \
    X_PROCESS_CONFIG(visuals.enable_fire_lighting, false);

typedef struct {
    const uint32_t *demo_ptr;
    const GF_LEVEL *level;

    struct {
        CONFIG config;
        GAME_BONUS_FLAG bonus_flag;
    } old_config;
} M_PRIV;

static int32_t m_LastDemoNum = 0;
static M_PRIV m_Priv;

static INPUT_STATE m_OldDemoInputDB = {};

static void M_PrepareConfig(M_PRIV *const p)
{
    p->old_config.config = g_Config;
    p->old_config.bonus_flag = Game_GetBonusFlag();
    Game_SetBonusFlag(GBF_NONE);
#define X_PROCESS_CONFIG(var, value) g_Config.var = value;
    L_MODIFY_CONFIG();
#undef X_PROCESS_CONFIG
}

static void M_RestoreConfig(M_PRIV *const p)
{
    Game_SetBonusFlag(p->old_config.bonus_flag);
#define X_PROCESS_CONFIG(var, value) g_Config.var = p->old_config.config.var;
    L_MODIFY_CONFIG();
#undef X_PROCESS_CONFIG
}

bool Demo_GetInput(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->demo_ptr == Demo_GetData()) {
        m_OldDemoInputDB = (INPUT_STATE) {};
    }

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
            uint32_t _pad:         6;
            uint32_t use_flare:    1;
            uint32_t menu_confirm: 1;
            uint32_t menu_back:    1;
            uint32_t save:         1;
            uint32_t load:         1;
            // clang-format on
        };
    } demo_input = { .any = *p->demo_ptr++ };

    if ((int32_t)demo_input.any == -1) {
        return false;
    }

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
        .use_flare    = demo_input.use_flare,
        .menu_confirm = demo_input.menu_confirm,
        .menu_back    = demo_input.menu_back,
        .save         = demo_input.save,
        .load         = demo_input.load,
        // clang-format on
    };

    g_InputDB.any = g_Input.any & ~m_OldDemoInputDB.any;
    m_OldDemoInputDB = g_Input;
    return true;
}

bool Demo_Start(const int32_t level_num)
{
    M_PRIV *const p = &m_Priv;
    p->level = GF_GetLevel(GFLT_DEMOS, level_num);
    ASSERT(p->level != nullptr);
    ASSERT(GF_GetCurrentLevel() == p->level);

    M_PrepareConfig(p);
    Interpolation_Remember();

    const uint32_t *const data = Demo_GetData();
    if (data == nullptr) {
        LOG_ERROR("Level '%s' has no demo data", p->level->path);
        return false;
    }

    if (p->level->music_track != MX_INACTIVE) {
        Music_Play_Direct(p->level->music_track, MPM_LOOPED);
    }

    p->demo_ptr = data;

    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara_item->pos.x = *p->demo_ptr++;
    lara_item->pos.y = *p->demo_ptr++;
    lara_item->pos.z = *p->demo_ptr++;
    lara_item->rot.x = *p->demo_ptr++;
    lara_item->rot.y = *p->demo_ptr++;
    lara_item->rot.z = *p->demo_ptr++;

    int16_t room_num = *p->demo_ptr++;
    Item_UpdateRoom(lara->item_num, room_num);
    const SECTOR *const sector = Room_GetSector(
        lara_item->pos.x, lara_item->pos.y, lara_item->pos.z, &room_num);
    lara_item->floor = Room_GetHeight(
        sector, lara_item->pos.x, lara_item->pos.y, lara_item->pos.z);

    lara->last_gun_type = *p->demo_ptr++;

    g_OverlayFlag = 1;
    Lara_Cheat_GetStuff();
    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);
    Camera_Initialise();

    Overlay_SetBottomTextPtr(GS_PTR(MISC_DEMO_MODE), true);
    return true;
}

void Demo_End(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Overlay_SetBottomText(nullptr, false);
    Music_Stop();
}

void Demo_Pause(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Overlay_SetBottomText(nullptr, false);
}

void Demo_Unpause(void)
{
    M_PRIV *const p = &m_Priv;
    M_PrepareConfig(p);
    Overlay_SetBottomTextPtr(GS_PTR(MISC_DEMO_MODE), true);
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
    return Game_Control(true);
}

void Demo_StopFlashing(void)
{
    Overlay_SetBottomTextPtr(GS_PTR(MISC_DEMO_MODE), false);
}
