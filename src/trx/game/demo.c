#include <trx/game/demo.h>

#include <trx/config.h>
#include <trx/core/file.h>
#include <trx/core/subsystem.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/game.h>
#include <trx/game/game_buf.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/gun.h>
#include <trx/game/gun/common.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/overlay.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

#define L_MODIFY_CONFIG()                                                      \
    X_PROCESS_CONFIG(gameplay.disable_healing_between_levels, false);          \
    X_PROCESS_CONFIG(gameplay.enable_fast_pickups, false);                     \
    X_PROCESS_CONFIG(gameplay.target_change_mode, TARGET_CHANGE_MODE_OFF);     \
    X_PROCESS_CONFIG(gameplay.enable_tr2_jumping, g_TRVersion >= 2);           \
    X_PROCESS_CONFIG(gameplay.enable_tr2_swim_cancel, g_TRVersion >= 2);       \
    X_PROCESS_CONFIG(gameplay.enable_tr2_swimming, g_TRVersion >= 2);          \
    X_PROCESS_CONFIG(gameplay.enable_wading, g_TRVersion >= 2);                \
    X_PROCESS_CONFIG(gameplay.enable_walk_to_items, false);                    \
    X_PROCESS_CONFIG(gameplay.fix_bear_ai, false);                             \
    X_PROCESS_CONFIG(gameplay.harpoon_recoil, 4);                              \
    X_PROCESS_CONFIG(                                                          \
        gameplay.look_mode,                                                    \
        g_TRVersion >= 2 ? LOOK_MODE_ENHANCED : LOOK_MODE_RESTRICTED);         \
    X_PROCESS_CONFIG(gameplay.start_lara_hitpoints, LARA_MAX_HITPOINTS);       \
    X_PROCESS_CONFIG(gameplay.target_mode, TARGET_LOCK_MODE_FULL);             \
    X_PROCESS_CONFIG(                                                          \
        gameplay.wall_glitch_mode,                                             \
        g_TRVersion >= 2 ? WALL_GLITCH_TR2 : WALL_GLITCH_TR1);                 \
    X_PROCESS_CONFIG(input.quick_guns_mode, QUICK_GUNS_MODE_DRAW_ONLY);        \
    X_PROCESS_CONFIG(visuals.enable_fire_lighting, false);                     \
    X_PROCESS_CONFIG(debug.enable_invulnerability, false);

typedef struct {
    const uint32_t *demo_ptr;
    const GF_LEVEL *level;
    GAME_BONUS_FLAG old_bonus_flag;
    uint32_t *data;
} M_PRIV;

static int32_t m_LastDemoNum = 0;
static M_PRIV m_Priv;

static void M_Shutdown(void)
{
    m_Priv = (M_PRIV) {};
    m_LastDemoNum = 0;
}

static void M_PrepareConfig(M_PRIV *const p)
{
    // Changing certains settings affects negatively the original game demo
    // data, so temporarily turn off all relevant enhancements.
    p->old_bonus_flag = Game_GetBonusFlag();
    Game_SetBonusFlag(GBF_NONE);
#define X_PROCESS_CONFIG(var, value)                                           \
    ASSERT(CONFIG_PUSH_HOLD(g_Config.var, value, CONFIG_HOLD_DEMO));
    L_MODIFY_CONFIG();
#undef X_PROCESS_CONFIG
    Config_Update();
}

static void M_RestoreConfig(M_PRIV *const p)
{
    Game_SetBonusFlag(p->old_bonus_flag);
#define X_PROCESS_CONFIG(var, value) ASSERT(CONFIG_POP_HOLD(g_Config.var));
    L_MODIFY_CONFIG();
#undef X_PROCESS_CONFIG
    Config_Update();
}

void Demo_LoadData(TRX_FILE *const file, const size_t size)
{
    M_PRIV *const p = &m_Priv;
    if (size == 0) {
        p->data = nullptr;
    } else {
        p->data =
            GameBuf_Alloc((size + 1) * sizeof(uint32_t), GBUF_DEMO_BUFFER);
        p->data[size] = -1;
        File_ReadData(file, p->data, size);
    }
}

bool Demo_UpdateInput(void)
{
    M_PRIV *const p = &m_Priv;
    const INPUT_STATE old_demo_input = g_Input;

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
    } demo_input = { .any = *p->demo_ptr };

    if ((int32_t)demo_input.any == -1) {
        return false;
    }

    // Translate demo inputs (that use hardcoded OG key layout) to TRX inputs.
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

    g_InputDB = g_Input;
    for (int32_t i = 0; i < INPUT_STATE_ANY_WORDS; i++) {
        g_InputDB.any[i] &= ~old_demo_input.any[i];
    }

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
    Interpolation_Remember();

    // Remember old inputs in case the demo was forcefully started with some
    // keys pressed. In that case, it should only be stopped if the user
    // presses some other key.
    Input_Update();

    if (p->data == nullptr) {
        LOG_ERROR("Level '%s' has no demo data", p->level->path);
        M_RestoreConfig(p);
        return false;
    }

    if (p->level->music_track != MX_INACTIVE) {
        Music_PlayBySlot(p->level->music_track, MPM_LOOP);
    }

    p->demo_ptr = p->data;

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
    const SECTOR *const sector = Room_GetSector(lara_item->pos, &room_num);
    lara_item->floor = Room_GetHeight(sector, lara_item->pos);

    if (g_TRVersion >= 2) {
        lara->last_gun_type = *p->demo_ptr++;
        Lara_Cheat_GetStuff();
    } else {
        lara->last_gun_type = Gun_GetDefaultType();
    }

    if (Gun_IsRifleType(lara->last_gun_type)) {
        Gun_SetLaraBackMesh(lara->last_gun_type);
    } else if (
        lara->last_gun_type != LGT_UNARMED
        && !Gun_IsFlareType(lara->last_gun_type)) {
        Gun_SetLaraHolsterLMesh(lara->last_gun_type);
        Gun_SetLaraHolsterRMesh(lara->last_gun_type);
    }

    Camera_Initialise();
    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);
    g_OverlayFlag = 1;

    Overlay_SetBottomText((OVERLAY_TEXT) {
        .kind = OVERLAY_TEXT_GS_KEY,
        .gs_key = GS_ID("general/misc/demo_mode"),
        .flash_enabled = true,
    });
    return true;
}

void Demo_End(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
    Music_Stop();
}

void Demo_Pause(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
}

void Demo_Unpause(void)
{
    M_PRIV *const p = &m_Priv;
    M_PrepareConfig(p);
    Overlay_SetBottomText((OVERLAY_TEXT) {
        .kind = OVERLAY_TEXT_GS_KEY,
        .gs_key = GS_ID("general/misc/demo_mode"),
        .flash_enabled = true,
    });
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
    Overlay_SetBottomText((OVERLAY_TEXT) {
        .kind = OVERLAY_TEXT_GS_KEY,
        .gs_key = GS_ID("general/misc/demo_mode"),
        .flash_enabled = false,
    });
}

REGISTER_SUBSYSTEM(.shutdown = M_Shutdown)
