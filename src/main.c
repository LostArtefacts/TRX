#include <stdio.h>
#include <windows.h>

#include "json_utils.h"
#include "mod.h"
#include "util.h"

#include "game/control.h"
#include "game/effects.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/health.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/setup.h"
#include "game/shell.h"

HINSTANCE hInstance = NULL;

static void TR1MInject()
{
    TR1MInjectControl();
    TR1MInjectDraw();
    TR1MInjectEffects();
    TR1MInjectGame();
    TR1MInjectHealth();
    TR1MInjectItems();
    TR1MInjectLOT();
    TR1MInjectLara();
    TR1MInjectLaraMisc();
    TR1MInjectSetup();
    TR1MInjectShell();
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
    TR1MConfig.enable_look_while_running =
        tr1m_json_get_boolean_value(json, "enable_look_while_running");
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
