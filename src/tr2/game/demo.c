#include "game/demo.h"

#include "decomp/savegame.h"
#include "game/camera.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/cheat.h"
#include "game/lara/control.h"
#include "game/level.h"
#include "game/music.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/stats.h"
#include "game/text.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/log.h>

typedef struct {
    const uint32_t *demo_ptr;
    const GF_LEVEL *level;
    TEXTSTRING *text;

    struct {
        bool bonus_flag;
    } old_config;
} M_PRIV;

static int32_t m_LastDemoNum = 0;
static M_PRIV m_Priv;

static INPUT_STATE m_OldDemoInputDB = {};

static void M_PrepareConfig(M_PRIV *p);
static void M_RestoreConfig(M_PRIV *p);

static void M_PrepareConfig(M_PRIV *const p)
{
    p->old_config.bonus_flag = g_SaveGame.bonus_flag;
    g_SaveGame.bonus_flag = false;
}

static void M_RestoreConfig(M_PRIV *const p)
{
    g_SaveGame.bonus_flag = p->old_config.bonus_flag;
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

    const uint32_t *const data = Demo_GetData();
    if (data == nullptr) {
        LOG_ERROR("Level '%s' has no demo data", p->level->path);
        return false;
    }

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

    g_Lara.last_gun_type = *p->demo_ptr++;

    g_OverlayStatus = 1;
    Lara_Cheat_GetStuff();
    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);
    Camera_Initialise();
    Stats_StartTimer();

    p->text = Text_Create(0, g_PhdWinHeight - 16, GS(MISC_DEMO_MODE));
    Text_Flash(p->text, true, 20);
    Text_CentreV(p->text, true);
    Text_CentreH(p->text, true);

    return true;
}

void Demo_End(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
    Text_Remove(p->text);
    Overlay_HideGameInfo();
    Sound_StopAll();
    Music_Stop();
    Music_SetVolume(g_Config.audio.music_volume);
    p->text = nullptr;
}

void Demo_Pause(void)
{
    M_PRIV *const p = &m_Priv;
    M_RestoreConfig(p);
}

void Demo_Unpause(void)
{
    M_PRIV *const p = &m_Priv;
    M_PrepareConfig(p);
    Stats_StartTimer();
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
    M_PRIV *const p = &m_Priv;
    Text_Flash(p->text, false, 0);
}
