#include "game/gameflow.h"

#include "decomp/decomp.h"
#include "decomp/stats.h"
#include "game/demo.h"
#include "game/game.h"
#include "game/gun/gun.h"
#include "game/inventory/backpack.h"
#include "game/overlay.h"
#include "game/requester.h"
#include "gameflow/gameflow_new.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <libtrx/benchmark.h>
#include <libtrx/game/objects/names.h>
#include <libtrx/memory.h>
#include <libtrx/virtual_file.h>

#include <stdio.h>

#define GF_CURRENT_VERSION 3

static void M_ReadStringTable(
    VFILE *file, int32_t count, char ***table, char **buffer);

static GF_ADD_INV M_ModifyInventory_GetGunAdder(LARA_GUN_TYPE gun_type);
static GF_ADD_INV M_ModifyInventory_GetAmmoAdder(LARA_GUN_TYPE gun_type);
static GF_ADD_INV M_ModifyInventory_GetItemAdder(GAME_OBJECT_ID object_id);
static void M_ModifyInventory_GunOrAmmo(
    START_INFO *start, int32_t type, LARA_GUN_TYPE gun_type);
static void M_ModifyInventory_Item(int32_t type, GAME_OBJECT_ID object_id);

static void M_ReadStringTable(
    VFILE *const file, const int32_t count, char ***const table,
    char **const buffer)
{
    VFile_Read(file, g_GF_LevelOffsets, sizeof(int16_t) * count);

    const int16_t buf_size = VFile_ReadS16(file);
    *buffer = Memory_Alloc(buf_size);
    VFile_Read(file, *buffer, buf_size);

    if (g_GameFlow.cyphered_strings) {
        for (int32_t i = 0; i < buf_size; i++) {
            (*buffer)[i] ^= g_GameFlow.cypher_code;
        }
    }

    *table = Memory_Alloc(sizeof(char *) * count);
    for (int32_t i = 0; i < count; i++) {
        const int32_t offset = g_GF_LevelOffsets[i];
        (*table)[i] = &(*buffer)[offset];
    }
}

static GF_ADD_INV M_ModifyInventory_GetGunAdder(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return GF_ADD_INV_PISTOLS;
    case LGT_MAGNUMS: return GF_ADD_INV_MAGNUMS;
    case LGT_UZIS:    return GF_ADD_INV_UZIS;
    case LGT_SHOTGUN: return GF_ADD_INV_SHOTGUN;
    case LGT_HARPOON: return GF_ADD_INV_HARPOON;
    case LGT_M16:     return GF_ADD_INV_M16;
    case LGT_GRENADE: return GF_ADD_INV_GRENADE;
    default:          return (GF_ADD_INV)-1;
    }
    // clang-format on
}

static GF_ADD_INV M_ModifyInventory_GetAmmoAdder(const LARA_GUN_TYPE gun_type)
{
    // clang-format off
    switch (gun_type) {
    case LGT_PISTOLS: return GF_ADD_INV_PISTOL_AMMO;
    case LGT_MAGNUMS: return GF_ADD_INV_MAGNUM_AMMO;
    case LGT_UZIS:    return GF_ADD_INV_UZI_AMMO;
    case LGT_SHOTGUN: return GF_ADD_INV_SHOTGUN_AMMO;
    case LGT_HARPOON: return GF_ADD_INV_HARPOON_AMMO;
    case LGT_M16:     return GF_ADD_INV_M16_AMMO;
    case LGT_GRENADE: return GF_ADD_INV_GRENADE_AMMO;
    default:          return (GF_ADD_INV)-1;
    }
    // clang-format on
}

static GF_ADD_INV M_ModifyInventory_GetItemAdder(const GAME_OBJECT_ID object_id)
{
    // clang-format off
    switch (object_id) {
    case O_FLARE_ITEM:          return GF_ADD_INV_FLARES;
    case O_SMALL_MEDIPACK_ITEM: return GF_ADD_INV_SMALL_MEDI;
    case O_LARGE_MEDIPACK_ITEM: return GF_ADD_INV_LARGE_MEDI;
    case O_PICKUP_ITEM_1:       return GF_ADD_INV_PICKUP_1;
    case O_PICKUP_ITEM_2:       return GF_ADD_INV_PICKUP_2;
    case O_PUZZLE_ITEM_1:       return GF_ADD_INV_PUZZLE_1;
    case O_PUZZLE_ITEM_2:       return GF_ADD_INV_PUZZLE_2;
    case O_PUZZLE_ITEM_3:       return GF_ADD_INV_PUZZLE_3;
    case O_PUZZLE_ITEM_4:       return GF_ADD_INV_PUZZLE_4;
    case O_KEY_ITEM_1:          return GF_ADD_INV_KEY_1;
    case O_KEY_ITEM_2:          return GF_ADD_INV_KEY_2;
    case O_KEY_ITEM_3:          return GF_ADD_INV_KEY_3;
    case O_KEY_ITEM_4:          return GF_ADD_INV_KEY_4;
    default:                    return (GF_ADD_INV)-1;
    }
    // clang-format on
}

static void M_ModifyInventory_GunOrAmmo(
    START_INFO *const start, const int32_t type, const LARA_GUN_TYPE gun_type)
{
    const GAME_OBJECT_ID gun_item = Gun_GetGunObject(gun_type);
    const GAME_OBJECT_ID ammo_item = Gun_GetAmmoObject(gun_type);
    const int32_t ammo_qty = Gun_GetAmmoQuantity(gun_type);
    AMMO_INFO *const ammo_info = Gun_GetAmmoInfo(gun_type);

    const GF_ADD_INV gun_adder = M_ModifyInventory_GetGunAdder(gun_type);
    const GF_ADD_INV ammo_adder = M_ModifyInventory_GetAmmoAdder(gun_type);

    if (Inv_RequestItem(gun_item)) {
        if (type == 1) {
            ammo_info->ammo += ammo_qty * g_GF_SecretInvItems[ammo_adder];
            for (int32_t i = 0; i < g_GF_SecretInvItems[ammo_adder]; i++) {
                Overlay_AddDisplayPickup(ammo_item);
            }
        } else if (type == 0) {
            ammo_info->ammo += ammo_qty * g_GF_Add2InvItems[ammo_adder];
        }
    } else if (
        (type == 0 && g_GF_Add2InvItems[gun_adder])
        || (type == 1 && g_GF_SecretInvItems[gun_adder])) {

        // clang-format off
        // TODO: consider moving this to Inv_AddItem
        switch (gun_type) {
        case LGT_PISTOLS: start->has_pistols = 1; break;
        case LGT_MAGNUMS: start->has_magnums = 1; break;
        case LGT_UZIS:    start->has_uzis = 1;    break;
        case LGT_SHOTGUN: start->has_shotgun = 1; break;
        case LGT_HARPOON: start->has_harpoon = 1; break;
        case LGT_M16:     start->has_m16 = 1;     break;
        case LGT_GRENADE: start->has_grenade = 1; break;
        default: break;
        }
        // clang-format on

        Inv_AddItem(gun_item);

        if (type == 1) {
            ammo_info->ammo += ammo_qty * g_GF_SecretInvItems[ammo_adder];
            Overlay_AddDisplayPickup(gun_item);
            for (int32_t i = 0; i < g_GF_SecretInvItems[ammo_adder]; i++) {
                Overlay_AddDisplayPickup(ammo_item);
            }
        } else if (type == 0) {
            ammo_info->ammo += ammo_qty * g_GF_Add2InvItems[ammo_adder];
        }
    } else if (type == 1) {
        for (int32_t i = 0; i < g_GF_SecretInvItems[ammo_adder]; i++) {
            Inv_AddItem(ammo_item);
            Overlay_AddDisplayPickup(ammo_item);
        }
    } else if (type == 0) {
        for (int32_t i = 0; i < g_GF_Add2InvItems[ammo_adder]; i++) {
            Inv_AddItem(ammo_item);
        }
    }
}

static void M_ModifyInventory_Item(
    const int32_t type, const GAME_OBJECT_ID object_id)
{
    const GF_ADD_INV item_adder = M_ModifyInventory_GetItemAdder(object_id);
    int32_t qty = 0;
    if (type == 1) {
        qty = g_GF_SecretInvItems[item_adder];
    } else if (type == 0) {
        qty = g_GF_Add2InvItems[item_adder];
    }

    for (int32_t i = 0; i < qty; i++) {
        Inv_AddItem(object_id);
        if (type == 1) {
            Overlay_AddDisplayPickup(object_id);
        }
    }
}

// TODO: inline me into GF_LoadScriptFile
BOOL __cdecl GF_LoadFromFile(const char *const file_name)
{
    BENCHMARK *const benchmark = Benchmark_Start();
    DWORD bytes_read;

    const char *full_path = GetFullPath(file_name);
    VFILE *const file = VFile_CreateFromPath(full_path);
    if (file == NULL) {
        return false;
    }

    g_GF_ScriptVersion = VFile_ReadS32(file);
    if (g_GF_ScriptVersion != GF_CURRENT_VERSION) {
        return false;
    }

    VFile_Read(file, g_GF_Description, 256);

    if (VFile_ReadS16(file) != sizeof(GAME_FLOW)) {
        return false;
    }
    g_GameFlow.first_option = VFile_ReadS32(file);
    g_GameFlow.title_replace = VFile_ReadS32(file);
    g_GameFlow.on_death_demo_mode = VFile_ReadS32(file);
    g_GameFlow.on_death_in_game = VFile_ReadS32(file);
    g_GameFlow.no_input_time = VFile_ReadS32(file);
    g_GameFlow.on_demo_interrupt = VFile_ReadS32(file);
    g_GameFlow.on_demo_end = VFile_ReadS32(file);
    VFile_Skip(file, 36);
    g_GameFlow.num_levels = VFile_ReadU16(file);
    g_GameFlow.num_pictures = VFile_ReadU16(file);
    g_GameFlow.num_titles = VFile_ReadU16(file);
    g_GameFlow.num_fmvs = VFile_ReadU16(file);
    g_GameFlow.num_cutscenes = VFile_ReadU16(file);
    g_GameFlow.num_demos = VFile_ReadU16(file);
    g_GameFlow.title_track = VFile_ReadU16(file);
    g_GameFlow.single_level = VFile_ReadS16(file);
    VFile_Skip(file, 32);

    const uint16_t flags = VFile_ReadU16(file);
    // clang-format off
    g_GameFlow.demo_version              = flags & 0x0001 ? 1 : 0;
    g_GameFlow.title_disabled            = flags & 0x0002 ? 1 : 0;
    g_GameFlow.cheat_mode_check_disabled = flags & 0x0004 ? 1 : 0;
    g_GameFlow.no_input_timeout          = flags & 0x0008 ? 1 : 0;
    g_GameFlow.load_save_disabled        = flags & 0x0010 ? 1 : 0;
    g_GameFlow.screen_sizing_disabled    = flags & 0x0020 ? 1 : 0;
    g_GameFlow.lockout_option_ring       = flags & 0x0040 ? 1 : 0;
    g_GameFlow.dozy_cheat_enabled        = flags & 0x0080 ? 1 : 0;
    g_GameFlow.cyphered_strings          = flags & 0x0100 ? 1 : 0;
    g_GameFlow.gym_enabled               = flags & 0x0200 ? 1 : 0;
    g_GameFlow.play_any_level            = flags & 0x0400 ? 1 : 0;
    g_GameFlow.cheat_enable              = flags & 0x0800 ? 1 : 0;
    // clang-format on
    VFile_Skip(file, 6);

    g_GameFlow.cypher_code = VFile_ReadU8(file);
    g_GameFlow.language = VFile_ReadU8(file);
    g_GameFlow.secret_track = VFile_ReadU8(file);
    g_GameFlow.level_complete_track = VFile_ReadU8(file);
    VFile_Skip(file, 4);

    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_LevelNames, &g_GF_LevelNamesBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_pictures, &g_GF_PicFilenames,
        &g_GF_PicFilenamesBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_titles, &g_GF_TitleFileNames,
        &g_GF_TitleFileNamesBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_fmvs, &g_GF_FMVFilenames, &g_GF_FMVFilenamesBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_LevelFileNames,
        &g_GF_LevelFileNamesBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_cutscenes, &g_GF_CutsceneFileNames,
        &g_GF_CutsceneFileNamesBuf);

    VFile_Read(
        file, &g_GF_LevelOffsets,
        sizeof(int16_t) * (g_GameFlow.num_levels + 1));
    {
        const int16_t size = VFile_ReadS16(file);
        g_GF_SequenceBuf = Memory_Alloc(size);
        VFile_Read(file, g_GF_SequenceBuf, size);
    }

    g_GF_FrontendSequence = g_GF_SequenceBuf;
    for (int32_t i = 0; i < g_GameFlow.num_levels; i++) {
        g_GF_ScriptTable[i] = g_GF_SequenceBuf + (g_GF_LevelOffsets[i + 1] / 2);
    }

    VFile_Read(file, g_GF_ValidDemos, sizeof(int16_t) * g_GameFlow.num_demos);

    if (VFile_ReadS16(file) != GF_S_GAME_NUMBER_OF) {
        return false;
    }

    M_ReadStringTable(
        file, GF_S_GAME_NUMBER_OF, &g_GF_GameStrings, &g_GF_GameStringsBuf);
    M_ReadStringTable(
        file, GF_S_PC_NUMBER_OF, &g_GF_PCStrings, &g_GF_PCStringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Puzzle1Strings,
        &g_GF_Puzzle1StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Puzzle2Strings,
        &g_GF_Puzzle2StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Puzzle3Strings,
        &g_GF_Puzzle3StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Puzzle4Strings,
        &g_GF_Puzzle4StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Pickup1Strings,
        &g_GF_Pickup1StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Pickup2Strings,
        &g_GF_Pickup2StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Key1Strings, &g_GF_Key1StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Key2Strings, &g_GF_Key2StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Key3Strings, &g_GF_Key3StringsBuf);
    M_ReadStringTable(
        file, g_GameFlow.num_levels, &g_GF_Key4Strings, &g_GF_Key4StringsBuf);

    VFile_Close(file);
    Benchmark_End(benchmark, NULL);
    return true;
}

int32_t __cdecl GF_LoadScriptFile(const char *const fname)
{
    g_GF_SunsetEnabled = false;

    if (!GF_LoadFromFile(fname)) {
        return false;
    }

    g_GameFlow.level_complete_track = MX_END_OF_LEVEL;

    Requester_SetHeading(
        &g_LoadGameRequester, g_GF_GameStrings[GF_S_GAME_PASSPORT_SELECT_LEVEL],
        0, 0, 0);
    Requester_SetHeading(
        &g_SaveGameRequester, g_GF_GameStrings[GF_S_GAME_PASSPORT_SELECT_LEVEL],
        0, 0, 0);

    return true;
}

int32_t __cdecl GF_DoFrontendSequence(void)
{
    GF_N_LoadStrings(-1);
    const GAME_FLOW_DIR dir =
        GF_InterpretSequence(g_GF_FrontendSequence, GFL_NORMAL, 1);
    return dir == GFD_EXIT_GAME;
}

int32_t __cdecl GF_DoLevelSequence(
    const int32_t start_level, const GAMEFLOW_LEVEL_TYPE type)
{
    GF_N_LoadStrings(start_level);

    int32_t current_level = start_level;
    while (true) {
        if (current_level > g_GameFlow.num_levels - 1) {
            g_IsTitleLoaded = false;
            return GFD_EXIT_TO_TITLE;
        }

        const int16_t *const ptr = g_GF_ScriptTable[current_level];
        const GAME_FLOW_DIR dir = GF_InterpretSequence(ptr, type, 0);
        current_level++;

        if (g_GameFlow.single_level >= 0) {
            return dir;
        }
        if ((dir & ~0xFF) != GFD_LEVEL_COMPLETE) {
            return dir;
        }
    }
}

int32_t __cdecl GF_InterpretSequence(
    const int16_t *ptr, GAMEFLOW_LEVEL_TYPE type, const int32_t seq_type)
{
    g_GF_NoFloor = false;
    g_GF_DeadlyWater = false;
    g_GF_SunsetEnabled = false;
    g_GF_LaraStartAnim = 0;
    g_GF_Kill2Complete = false;
    g_GF_RemoveAmmo = false;
    g_GF_RemoveWeapons = false;

    for (int32_t i = 0; i < GF_ADD_INV_NUMBER_OF; i++) {
        g_GF_SecretInvItems[i] = 0;
        g_GF_Add2InvItems[i] = 0;
    }

    g_GF_MusicTracks[0] = 2;
    g_CineTargetAngle = PHD_90;
    g_GF_NumSecrets = 3;

    int32_t ntracks = 0;
    GAME_FLOW_DIR dir = GFD_EXIT_TO_TITLE;

    while (*ptr != GFE_END_SEQ) {
        switch (*ptr) {
        case GFE_PICTURE:
            ptr += 2;
            break;

        case GFE_LIST_START:
        case GFE_LIST_END:
            ptr++;
            break;

        case GFE_PLAY_FMV:
            if (type != GFL_SAVED) {
                if (ptr[2] == GFE_PLAY_FMV) {
                    if (S_IntroFMV(
                            g_GF_FMVFilenames[ptr[1]],
                            g_GF_FMVFilenames[ptr[3]])) {
                        return GFD_EXIT_GAME;
                    }
                    ptr += 2;
                } else if (S_PlayFMV(g_GF_FMVFilenames[ptr[1]])) {
                    return GFD_EXIT_GAME;
                }
            }
            ptr += 2;
            break;

        case GFE_START_LEVEL:
            if (ptr[1] > g_GameFlow.num_levels) {
                dir = GFD_EXIT_TO_TITLE;
            } else if (type != GFL_STORY) {
                if (type == GFL_MID_STORY) {
                    return GFD_EXIT_TO_TITLE;
                }
                dir = Game_Start(ptr[1], type);
                g_GF_StartGame = 0;
                if (type == GFL_SAVED) {
                    type = GFL_NORMAL;
                }
                if ((dir & ~0xFF) != GFD_LEVEL_COMPLETE) {
                    return dir;
                }
            }
            ptr += 2;
            break;

        case GFE_CUTSCENE:
            if (type != GFL_SAVED) {
                const int16_t level = g_CurrentLevel;
                const int32_t result = Game_Cutscene_Start(ptr[1]);
                g_CurrentLevel = level;
                // TODO: make Game_Cutscene_Start return GAME_FLOW_DIR
                if (result == 2
                    && (type == GFL_STORY || type == GFL_MID_STORY)) {
                    return GFD_EXIT_TO_TITLE;
                }
                if (result == 3) {
                    return GFD_EXIT_GAME;
                }
                if (result == 4) {
                    dir = g_GF_OverrideDir;
                    g_GF_OverrideDir = (GAME_FLOW_DIR)-1;
                    return dir;
                }
            }
            ptr += 2;
            break;

        case GFE_LEVEL_COMPLETE:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                if (LevelStats(g_CurrentLevel)) {
                    return GFD_EXIT_TO_TITLE;
                }
                dir = GFD_START_GAME | (g_CurrentLevel + 1);
            }
            ptr++;
            break;

        case GFE_DEMO_PLAY:
            if (type != GFL_SAVED && type != GFL_STORY
                && type != GFL_MID_STORY) {
                return Demo_Start(ptr[1]);
            }
            ptr += 2;
            break;

        case GFE_JUMP_TO_SEQ:
            ptr += 2;
            break;

        case GFE_SET_TRACK:
            g_GF_MusicTracks[ntracks] = ptr[1];
            Game_SetCutsceneTrack(ptr[1]);
            ntracks++;
            ptr += 2;
            break;

        case GFE_SUNSET:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                g_GF_SunsetEnabled = true;
            }
            ptr++;
            break;

        case GFE_LOADING_PIC:
            ptr += 2;
            break;

        case GFE_DEADLY_WATER:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                g_GF_DeadlyWater = true;
            }
            ptr++;
            break;

        case GFE_REMOVE_WEAPONS:
            if (type != GFL_STORY && type != GFL_MID_STORY
                && type != GFL_SAVED) {
                g_GF_RemoveWeapons = true;
            }
            ptr++;
            break;

        case GFE_GAME_COMPLETE:
            DisplayCredits();
            if (GameStats(g_CurrentLevel)) {
                return GFD_EXIT_TO_TITLE;
            }
            dir = GFD_EXIT_TO_TITLE;
            ptr++;
            break;

        case GFE_CUT_ANGLE:
            if (type != GFL_SAVED) {
                g_CineTargetAngle = ptr[1];
            }
            ptr += 2;
            break;

        case GFE_NO_FLOOR:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                g_GF_NoFloor = ptr[1];
            }
            ptr += 2;
            break;

        case GFE_ADD_TO_INV:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                if (ptr[1] < 1000) {
                    g_GF_SecretInvItems[ptr[1]]++;
                } else if (type != GFL_SAVED) {
                    g_GF_Add2InvItems[ptr[1] - 1000]++;
                }
            }
            ptr += 2;
            break;

        case GFE_START_ANIM:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                g_GF_LaraStartAnim = ptr[1];
            }
            ptr += 2;
            break;

        case GFE_NUM_SECRETS:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                g_GF_NumSecrets = ptr[1];
            }
            ptr += 2;
            break;

        case GFE_KILL_TO_COMPLETE:
            if (type != GFL_STORY && type != GFL_MID_STORY) {
                g_GF_Kill2Complete = true;
            }
            ptr++;
            break;

        case GFE_REMOVE_AMMO:
            if (type != GFL_STORY && type != GFL_MID_STORY
                && type != GFL_SAVED) {
                g_GF_RemoveAmmo = true;
            }
            ptr++;
            break;

        default:
            return GFD_EXIT_GAME;
        }
    }

    if (type == GFL_STORY || type == GFL_MID_STORY) {
        return 0;
    }
    return dir;
}

void __cdecl GF_ModifyInventory(const int32_t level, const int32_t type)
{
    START_INFO *const start = &g_SaveGame.start[level];

    if (!start->has_pistols && g_GF_Add2InvItems[GF_ADD_INV_PISTOLS]) {
        start->has_pistols = 1;
        Inv_AddItem(O_PISTOL_ITEM);
    }

    M_ModifyInventory_GunOrAmmo(start, type, LGT_MAGNUMS);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_UZIS);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_SHOTGUN);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_HARPOON);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_M16);
    M_ModifyInventory_GunOrAmmo(start, type, LGT_GRENADE);

    M_ModifyInventory_Item(type, O_FLARE_ITEM);
    M_ModifyInventory_Item(type, O_SMALL_MEDIPACK_ITEM);
    M_ModifyInventory_Item(type, O_LARGE_MEDIPACK_ITEM);
    M_ModifyInventory_Item(type, O_PICKUP_ITEM_1);
    M_ModifyInventory_Item(type, O_PICKUP_ITEM_2);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_1);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_2);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_3);
    M_ModifyInventory_Item(type, O_PUZZLE_ITEM_4);
    M_ModifyInventory_Item(type, O_KEY_ITEM_1);
    M_ModifyInventory_Item(type, O_KEY_ITEM_2);
    M_ModifyInventory_Item(type, O_KEY_ITEM_3);
    M_ModifyInventory_Item(type, O_KEY_ITEM_4);

    for (int32_t i = 0; i < GF_ADD_INV_NUMBER_OF; i++) {
        if (type == 1) {
            g_GF_SecretInvItems[i] = 0;
        } else if (type == 0) {
            g_GF_Add2InvItems[i] = 0;
        }
    }
}
