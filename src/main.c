#include <stdio.h>
#include <windows.h>

#include "json_utils.h"
#include "mod.h"
#include "util.h"

#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/health.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/setup.h"
#include "game/text.h"
#include "specific/file.h"
#include "specific/game.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"

HINSTANCE hInstance = NULL;

static void TR1MInject()
{
    TR1MInjectGameControl();
    TR1MInjectGameDraw();
    TR1MInjectGameEffects();
    TR1MInjectGameHealth();
    TR1MInjectGameItems();
    TR1MInjectGameLOT();
    TR1MInjectGameLara();
    TR1MInjectGameLaraMisc();
    TR1MInjectGameLaraSurf();
    TR1MInjectGameLaraSwim();
    TR1MInjectGameSetup();
    TR1MInjectGameText();
    TR1MInjectSpecificFile();
    TR1MInjectSpecificGame();
    TR1MInjectSpecificInit();
    TR1MInjectSpecificInput();
    TR1MInjectSpecificOutput();
}

static int TR1MReadConfig()
{
    FILE* fp = fopen("TR1Main.json", "rb");
    if (!fp) {
        return 0;
    }

    fseek(fp, 0, SEEK_END);
    int cfg_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* cfg_data = malloc(cfg_size);
    if (!cfg_data) {
        fclose(fp);
        return 0;
    }
    fread(cfg_data, 1, cfg_size, fp);
    fclose(fp);

    json_value* json = json_parse((const json_char*)cfg_data, cfg_size);

    TR1MData.medipack_cooldown = 0;

    TR1MConfig.disable_healing_between_levels =
        tr1m_json_get_boolean_value(json, "disable_healing_between_levels");
    TR1MConfig.disable_medpacks =
        tr1m_json_get_boolean_value(json, "disable_medpacks");
    TR1MConfig.disable_magnums =
        tr1m_json_get_boolean_value(json, "disable_magnums");
    TR1MConfig.disable_uzis = tr1m_json_get_boolean_value(json, "disable_uzis");
    TR1MConfig.disable_shotgun =
        tr1m_json_get_boolean_value(json, "disable_shotgun");
    TR1MConfig.enable_red_healthbar =
        tr1m_json_get_boolean_value(json, "enable_red_healthbar");
    TR1MConfig.enable_enemy_healthbar =
        tr1m_json_get_boolean_value(json, "enable_enemy_healthbar");
    TR1MConfig.enable_enhanced_look =
        tr1m_json_get_boolean_value(json, "enable_enhanced_look");
    TR1MConfig.enable_enhanced_ui =
        tr1m_json_get_boolean_value(json, "enable_enhanced_ui");

    const char* healthbar_showing_mode =
        tr1m_json_get_string_value(json, "healthbar_showing_mode");
    TR1MConfig.healthbar_showing_mode = TR1M_BSM_DEFAULT;
    if (healthbar_showing_mode) {
        if (!strcmp(healthbar_showing_mode, "flashing")) {
            TR1MConfig.healthbar_showing_mode = TR1M_BSM_FLASHING;
        } else if (!strcmp(healthbar_showing_mode, "always")) {
            TR1MConfig.healthbar_showing_mode = TR1M_BSM_ALWAYS;
        }
    }

    TR1MConfig.enable_numeric_keys =
        tr1m_json_get_boolean_value(json, "enable_numeric_keys");
    TR1MConfig.fix_end_of_level_freeze =
        tr1m_json_get_boolean_value(json, "fix_end_of_level_freeze");
    TR1MConfig.fix_tihocan_secret_sound =
        tr1m_json_get_boolean_value(json, "fix_tihocan_secret_sound");

    json_value_free(json);
    free(cfg_data);
    return 1;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        freopen("./TR1Main.log", "w", stdout);
        TR1MReadConfig();
        TRACE("Attached");
        hInstance = hinstDLL;
        TR1MInject();
        break;

    case DLL_PROCESS_DETACH:
        TRACE("Detached");
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
