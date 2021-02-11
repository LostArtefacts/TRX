#include <stdio.h>
#include <windows.h>

#include "func.h"
#include "json_utils.h"
#include "mod.h"
#include "struct.h"
#include "util.h"

HINSTANCE hInstance = NULL;

static void tr1m_inject()
{
    INJECT(0x0041AF90, S_LoadLevel);
    INJECT(0x0041B3F0, LoadRooms);
    INJECT(0x0041BC60, LoadItems);
    INJECT(0x0041BFC0, GetFullPath);
    INJECT(0x0041C020, FindCdDrive);
    INJECT(0x0041D5A0, LevelStats);
    INJECT(0x0041D8F0, GetRandomControl);
    INJECT(0x0041D910, SeedRandomControl);
    INJECT(0x0041DD00, DrawGameInfo);
    INJECT(0x0041DEA0, DrawHealthBar);
    INJECT(0x0041DF50, DrawAmmoInfo);
    INJECT(0x0041E2C0, init_game_malloc);
    INJECT(0x0041E3B0, game_free);
    INJECT(0x00422250, InitialiseFXArray);
    INJECT(0x00428020, InitialiseLara);
    INJECT(0x0042A2C0, DB_Log);
    INJECT(0x0042A300, InitialiseLOTArray);
    INJECT(0x004302D0, S_DrawHealthBar);
    INJECT(0x00430450, S_DrawAirBar);
}

static int tr1m_read_config()
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
    TR1MConfig.enable_red_healthbar =
        tr1m_json_get_boolean_value(json, "enable_red_healthbar");
    TR1MConfig.enable_enemy_healthbar =
        tr1m_json_get_boolean_value(json, "enable_enemy_healthbar");
    TR1MConfig.fix_end_of_level_freeze =
        tr1m_json_get_boolean_value(json, "fix_end_of_level_freeze");

    json_value_free(json);
    free(cfg_data);
    return 1;
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        freopen("./TR1Main.log", "w", stdout);
        tr1m_read_config();
        TRACE("Attached");
        hInstance = hinstDLL;
        tr1m_inject();
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
