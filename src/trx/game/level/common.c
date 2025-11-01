#include <trx/benchmark.h>
#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/effects.h>
#include <trx/game/game.h>
#include <trx/game/game_string_table.h>
#include <trx/game/gym.h>
#include <trx/game/lara.h>
#include <trx/game/level.h>
#include <trx/game/music.h>
#include <trx/game/option.h>
#include <trx/game/output.h>
#include <trx/game/overlay.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/savegame.h>
#include <trx/game/sound.h>
#include <trx/log.h>

void Level_Unload(void)
{
    Music_ResetTrackFlags();
    Sound_ResetSamples();

    Lara_InitialiseLoad(NO_ITEM);

#if TR_VERSION == 2
    Gym_ResetAssault();
    Creature_SetAlliesHostile(false);
#endif
    Object_Reset();
    Camera_Reset();
    Walkable_Reset();

#if TR_VERSION == 2
    Output_SetSunsetTimer(0);
#endif
    Output_DispatchLevelUnload();

    Music_SetVolume(g_Config.audio.music_volume);
    Sound_StopAll();
    Viewport_AlterFOV(-1);
}

bool Level_Initialise(
    const GF_LEVEL *const level, const GF_SEQUENCE_CONTEXT seq_ctx)
{
    BENCHMARK benchmark = Benchmark_Start();
    LOG_DEBUG("num=%d (%s)", level->num, level->path);
    if (level->type == GFL_DEMO) {
        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);
    }

    Game_SetIsLevelComplete(false);
    Game_FadeToBlack(-1);
    if (level->type != GFL_TITLE && level->type != GFL_DEMO) {
        Gym_SetInventoryOpenEnabled(false);
    }
    if (level->type != GFL_TITLE && level->type != GFL_CUTSCENE) {
        Game_SetCurrentLevel(level);
    }
    GF_SetCurrentLevel(level);

    if (level->type != GFL_TITLE) {
        // TODO: move me elsewhere
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        if (resume != nullptr) {
            resume->stats.timer = 0;
            resume->stats.secret_flags = 0;
            resume->stats.secret_count = 0;
            resume->stats.pickup_count = 0;
            resume->stats.kill_count = 0;
            resume->stats.ammo_hits = 0;
            resume->stats.ammo_used = 0;
            resume->stats.medipacks_used = 0;
            resume->stats.distance_travelled = 0;
        }
    }

    if (level == nullptr) {
        return false;
    }

    Level_Unload();
    Level_Load(level);

    GameStringTable_Apply(level);

    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Option_Reset();
    Overlay_Reset();
    Overlay_SetHealthBarTimer(100);

    Benchmark_End(&benchmark, nullptr);
    return true;
}
