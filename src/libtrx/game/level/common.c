#include "benchmark.h"
#include "config.h"
#include "debug.h"
#include "game/camera.h"
#include "game/creature.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_string_table.h"
#include "game/gym.h"
#include "game/lara.h"
#include "game/level.h"
#include "game/music.h"
#include "game/option.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/pathing.h"
#include "game/random.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/viewport.h"
#include "log.h"

static void M_DisableObject(const OBJECT_ID object_id)
{
    OBJECT *const obj = Object_Get(object_id);
    obj->loaded = false;
    obj->collision_func = nullptr;
    obj->control_func = nullptr;
    obj->draw_func = nullptr;
    obj->floor_height_func = nullptr;
    obj->ceiling_height_func = nullptr;
}

static void M_ReplaceObject(
    const OBJECT_ID src_object_id, const OBJECT_ID dst_object_id)
{
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->object_id == src_object_id) {
            item->object_id = dst_object_id;
        }
    }
}

static void M_DisableObjectsIfNeeded(void)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    if (level == nullptr
        || (level->type != GFL_NORMAL && level->type != GFL_BONUS
            && level->type != GFL_GYM)) {
        return;
    }
    if (g_Config.gameplay.disable_medpacks) {
        M_DisableObject(O_SMALL_MEDIPACK_ITEM);
        M_DisableObject(O_LARGE_MEDIPACK_ITEM);
    }

    if (g_Config.gameplay.disable_extra_guns) {
        const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        ASSERT(resume != nullptr);
        for (int32_t i = 0; g_GunObjects[i] != NO_OBJECT; i++) {
            if (resume->flags.has_pistols) {
                M_DisableObject(g_GunObjects[i]);
            } else {
                M_ReplaceObject(g_GunObjects[i], O_PISTOL_ITEM);
            }
            M_DisableObject(
                Object_GetCognate(g_GunObjects[i], g_GunAmmoObjectMap));
        }
    }
}

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
#if TR_VERSION == 1
            resume->stats.pickup_count = 0;
#endif
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
    M_DisableObjectsIfNeeded();

    GameStringTable_Apply(level);

    Effect_InitialiseArray();
    LOT_InitialiseArray();

    Option_Reset();
    Overlay_Reset();
    Overlay_SetHealthBarTimer(100);

    Benchmark_End(&benchmark, nullptr);
    return true;
}
