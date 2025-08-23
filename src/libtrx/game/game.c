#include "game/game.h"

#include "game/const.h"
#include "game/fmv.h"
#include "game/game_flow.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "game/ui.h"

static bool m_IsPlaying = false;
static const GF_LEVEL *m_CurrentLevel = nullptr;
static GAME_BONUS_FLAG m_BonusFlag = GBF_NONE;
static bool m_IsLevelComplete = false;
static FADER m_Fader;

void Game_SetIsPlaying(const bool is_playing)
{
    m_IsPlaying = is_playing;
    Random_FreezeDraw(!is_playing);
}

bool Game_IsPlaying(void)
{
    return m_IsPlaying;
}

const GF_LEVEL *Game_GetCurrentLevel(void)
{
    return m_CurrentLevel;
}

void Game_SetCurrentLevel(const GF_LEVEL *const level)
{
    m_CurrentLevel = level;
}

bool Game_IsInGym(void)
{
    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    return current_level != nullptr && current_level->type == GFL_GYM;
}

bool Game_IsLoaded(void)
{
    if (FMV_IsPlaying()) {
        return false;
    }
    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level == nullptr || current_level->type == GFL_TITLE) {
        return false;
    }
    return true;
}

bool Game_IsPlayable(void)
{
    if (FMV_IsPlaying()) {
        return false;
    }

    const GF_LEVEL *const current_level = GF_GetCurrentLevel();
    if (current_level == nullptr || current_level->type == GFL_TITLE
        || current_level->type == GFL_DEMO
        || current_level->type == GFL_CUTSCENE) {
        return false;
    }

    if (!Object_Get(O_LARA)->loaded || Lara_GetItem() == nullptr
        || !Lara_IsControllable()) {
        return false;
    }

    return true;
}

GAME_BONUS_FLAG Game_GetBonusFlag(void)
{
    return m_BonusFlag;
}

void Game_SetBonusFlag(const GAME_BONUS_FLAG flag)
{
    m_BonusFlag = flag;
}

bool Game_IsBonusFlagSet(const GAME_BONUS_FLAG flag)
{
    return (m_BonusFlag & flag) != 0;
}

void Game_SetIsLevelComplete(const bool is_complete)
{
    m_IsLevelComplete = is_complete;
}

bool Game_IsLevelComplete(void)
{
    return m_IsLevelComplete;
}

void Game_FadeToBlack(const int32_t duration)
{
    Fader_Init(
        &m_Fader, FADER_TRANSPARENT, FADER_BLACK, duration / (float)LOGIC_FPS);
}

void Game_DrawFade(void)
{
    UI_BeginFade(&m_Fader, true);
    UI_EndFade();
}
