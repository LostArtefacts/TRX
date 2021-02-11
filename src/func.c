#include "func.h"
#include "data.h"
#include "mod.h"
#include "struct.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

typedef enum {
    TRM1_BAR_LARA_HEALTH = 0,
    TRM1_BAR_LARA_AIR = 1,
    TRM1_BAR_ENEMY_HEALTH = 2,
    TRM1_BAR_NUMBER = 3,
} TR1M_BAR;

static int TR1MGetOverlayScale(int base)
{
    double result = PhdWinWidth;
    result *= base;
    result /= 800.0;

    // only scale up, not down
    if (result < base) {
        result = base;
    }

    return round(result);
}

static void TR1MRenderBar(int value, int value_max, int bar_type)
{
    const int p1 = -100;
    const int p2 = -200;
    const int p3 = -300;
    const int p4 = -400;
    const int percent_max = 100;

    if (value < 0) {
        value = 0;
    } else if (value > value_max) {
        value = value_max;
    }
    int percent = value * 100 / value_max;

#define COLOR_BAR_SIZE 5
    int color_bar[TRM1_BAR_NUMBER][COLOR_BAR_SIZE] = {
        { 8, 11, 8, 6, 24 },
        { 32, 41, 32, 19, 21 },
        { 18, 17, 18, 19, 21 },
    };
    if (TR1MConfig.enable_red_healthbar) {
        color_bar[TRM1_BAR_LARA_HEALTH][0] = 29;
        color_bar[TRM1_BAR_LARA_HEALTH][1] = 30;
        color_bar[TRM1_BAR_LARA_HEALTH][2] = 29;
        color_bar[TRM1_BAR_LARA_HEALTH][3] = 28;
        color_bar[TRM1_BAR_LARA_HEALTH][4] = 26;
    }

    const int color_border_1 = 19;
    const int color_border_2 = 17;
    const int color_bgnd = 0;

    int scale = TR1MGetOverlayScale(1.0);
    int width = percent_max * scale;
    int height = 5 * scale;

    int x = 8 * scale;
    int y = 8 * scale;

    if (bar_type == TRM1_BAR_LARA_AIR) {
        // place air bar on the right
        x = PhdWinWidth - width - x;
    } else if (bar_type == TRM1_BAR_ENEMY_HEALTH) {
        // place enemy bar on the bottom
        y = PhdWinHeight - height - y;
    }

    int padding = 2;
    int top = y - padding;
    int left = x - padding;
    int bottom = top + height + padding + 1;
    int right = left + width + padding + 1;

    // background
    for (int i = 1; i < height + 3; i++) {
        Insert2DLine(left + 1, top + i, right, top + i, p1, color_bgnd);
    }

    // top / left border
    Insert2DLine(left, top, right + 1, top, p2, color_border_1);
    Insert2DLine(left, top, left, bottom, p2, color_border_1);

    // bottom / right border
    Insert2DLine(left + 1, bottom, right, bottom, p2, color_border_2);
    Insert2DLine(right, top, right, bottom, p2, color_border_2);

    const int blink_interval = 20;
    const int blink_threshold = bar_type == TRM1_BAR_ENEMY_HEALTH ? 0 : 20;
    int blink_time = SaveGame[0].timer % blink_interval;
    int blink = percent <= blink_threshold && blink_time > blink_interval / 2;

    if (percent && !blink) {
        width -= (percent_max - percent) * scale;

        top = y;
        left = x;
        bottom = top + height;
        right = left + width;

        for (int i = 0; i < height; i++) {
            int color_index = i * COLOR_BAR_SIZE / height;
            Insert2DLine(
                left, top + i, right, top + i, p4,
                color_bar[bar_type][color_index]);
        }
    }
}

void __cdecl init_game_malloc()
{
    TRACE("");
    GameAllocMemPointer = GameMemoryPointer;
    GameAllocMemFree = GameMemorySize;
    GameAllocMemUsed = 0;
}

void __cdecl game_free(int free_size)
{
    TRACE("");
    GameAllocMemPointer -= free_size;
    GameAllocMemFree += free_size;
    GameAllocMemUsed -= free_size;
}

void __cdecl DB_Log(char* a1, ...)
{
    va_list va;
    char buffer[256] = { 0 };

    va_start(va, a1);
    if (!dword_45A1F0) {
        vsprintf(buffer, a1, va);
        TRACE(buffer);
        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }
}

const char* GetFullPath(const char* filename)
{
    TRACE(filename);
#if defined FEATURE_NOCD_DATA
    sprintf(newpath, ".\\%s", filename);
#else
    if (DEMO)
        sprintf(newpath, "%c:\\tomb\\%s", cd_drive, filename);
    else
        sprintf(newpath, "%c:\\%s", cd_drive, filename);
#endif
    return newpath;
}

int FindCdDrive()
{
    TRACE("");
    FILE* fp;
    char root[5] = "C:\\";
    char tmp_path[MAX_PATH];

    for (cd_drive = 'C'; cd_drive <= 'Z'; cd_drive++) {
        root[0] = cd_drive;
        if (GetDriveType(root) == DRIVE_CDROM) {
            sprintf(tmp_path, "%c:\\tomb\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 1;
                return fclose(fp);
            }
            sprintf(tmp_path, "%c:\\data\\title.phd", cd_drive);
            fp = fopen(tmp_path, "rb");
            if (fp) {
                DEMO = 0;
                return fclose(fp);
            }
        }
    }
    ShowFatalError("ERROR: Please insert TombRaider CD");
    return 0;
}

int LoadRooms(FILE* fp)
{
    TRACE("");
    uint16_t count2;
    uint32_t count4;

    _fread(&RoomCount, sizeof(uint16_t), 1, fp);
    if (RoomCount > MAX_ROOMS) {
        strcpy(StringToShow, "LoadRoom(): Too many rooms");
        return 0;
    }

    RoomInfo = game_malloc(sizeof(ROOM_INFO) * RoomCount, GBUF_RoomInfos);
    if (!RoomInfo) {
        strcpy(StringToShow, "LoadRoom(): Could not allocate memory for rooms");
        return 0;
    }

    int i = 0;
    for (ROOM_INFO* current_room_info = RoomInfo; i < RoomCount;
         ++i, ++current_room_info) {
        // Room position
        _fread(&current_room_info->x, sizeof(uint32_t), 1, fp);
        current_room_info->y = 0;
        _fread(&current_room_info->z, sizeof(uint32_t), 1, fp);

        // Room floor/ceiling
        _fread(&current_room_info->min_floor, sizeof(uint32_t), 1, fp);
        _fread(&current_room_info->max_ceiling, sizeof(uint32_t), 1, fp);

        // Room mesh
        _fread(&count4, sizeof(uint32_t), 1, fp);
        current_room_info->data =
            game_malloc(sizeof(uint16_t) * count4, GBUF_RoomMesh);
        _fread(current_room_info->data, sizeof(uint16_t), count4, fp);

        // Doors
        _fread(&count2, sizeof(uint16_t), 1, fp);
        if (!count2) {
            current_room_info->doors = NULL;
        } else {
            current_room_info->doors = game_malloc(
                sizeof(uint16_t) + sizeof(DOOR_INFO) * count2, GBUF_RoomDoor);
            current_room_info->doors->count = count2;
            _fread(
                &current_room_info->doors->door, sizeof(DOOR_INFO), count2, fp);
        }

        // Room floor
        _fread(&current_room_info->x_size, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->y_size, sizeof(uint16_t), 1, fp);
        count4 = current_room_info->y_size * current_room_info->x_size;
        current_room_info->floor =
            game_malloc(sizeof(FLOOR_INFO) * count4, GBUF_RoomFloor);
        _fread(current_room_info->floor, sizeof(FLOOR_INFO), count4, fp);

        // Room lights
        _fread(&current_room_info->ambient, sizeof(uint16_t), 1, fp);
        _fread(&current_room_info->num_lights, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_lights) {
            current_room_info->light = NULL;
        } else {
            current_room_info->light = game_malloc(
                sizeof(LIGHT_INFO) * current_room_info->num_lights,
                GBUF_RoomLights);
            _fread(
                current_room_info->light, sizeof(LIGHT_INFO),
                current_room_info->num_lights, fp);
        }

        // Static mesh infos
        _fread(&current_room_info->num_meshes, sizeof(uint16_t), 1, fp);
        if (!current_room_info->num_meshes) {
            current_room_info->mesh = NULL;
        } else {
            current_room_info->mesh = game_malloc(
                sizeof(MESH_INFO) * current_room_info->num_meshes,
                GBUF_RoomStaticMeshInfos);
            _fread(
                current_room_info->mesh, sizeof(MESH_INFO),
                current_room_info->num_meshes, fp);
        }

        // Flipped (alternative) room
        _fread(&current_room_info->flipped_room, sizeof(uint16_t), 1, fp);

        // Room flags
        _fread(&current_room_info->flags, sizeof(uint16_t), 1, fp);

        // Initialise some variables
        current_room_info->bound_active = 0;
        current_room_info->bound_left = PhdWinMaxX;
        current_room_info->bound_top = PhdWinMaxY;
        current_room_info->bound_bottom = 0;
        current_room_info->bound_right = 0;
        current_room_info->item_number = -1;
        current_room_info->fx_number = -1;
    }

    _fread(&count4, sizeof(uint32_t), 1, fp);
    FloorData = game_malloc(sizeof(uint16_t) * count4, GBUF_FloorData);
    _fread(FloorData, sizeof(uint16_t), count4, fp);

    return 1;
}

void __cdecl LevelStats(int level_id)
{
    TRACE("");

    if (TR1MConfig.disable_healing_between_levels) {
        TR1MData.stored_lara_health =
            LaraItem ? LaraItem->hit_points : LARA_HITPOINTS;
    }

    static char string[100];
    TEXTSTRING* txt;

    TempVideoAdjust(HiRes, 1.0);
    T_InitPrint();

    // heading
    sprintf(string, "%s", LevelTitles[level_id]); // TODO: translation
    txt = T_Print(0, -50, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // time taken
    int seconds = SaveGame[0].timer / 30;
    int hours = seconds / 3600;
    int minutes = (seconds / 60) % 60;
    seconds %= 60;
    if (hours) {
        sprintf(
            string, "%s %d:%d%d:%d%d",
            "TIME TAKEN", // TODO: translation
            hours, minutes / 10, minutes % 10, seconds / 10, seconds % 10);
    } else {
        sprintf(
            string, "%s %d:%d%d",
            "TIME TAKEN", // TODO: translation
            minutes, seconds / 10, seconds % 10);
    }
    txt = T_Print(0, 70, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // secrets
    int secrets_taken = 0;
    int secrets_total = MAX_SECRETS;
    do {
        if (SaveGame[0].secrets & 1) {
            ++secrets_taken;
        }
        SaveGame[0].secrets >>= 1;
        --secrets_total;
    } while (secrets_total);
    sprintf(
        string, "%s %d %s %d",
        "SECRETS", // TODO: translation
        secrets_taken,
        "OF", // TODO: translation
        SecretTotals[level_id] // TODO: calculate this automatically
    );
    txt = T_Print(0, 40, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // pickups
    sprintf(
        string, "%s %d", "PICKUPS",
        SaveGame[0].pickups); // TODO: translation
    txt = T_Print(0, 10, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // kills
    sprintf(string, "%s %d", "KILLS", SaveGame[0].kills); // TODO: translation
    txt = T_Print(0, -20, 0, string);
    T_CentreH(txt, 1);
    T_CentreV(txt, 1);

    // wait till action key release
    if (TR1MConfig.fix_end_of_level_freeze) {
        while (InputStatus & IN_SELECT) {
            S_UpdateInput();
            S_InitialisePolyList();
            S_CopyBufferToScreen();
            S_UpdateInput();
            T_DrawText();
            S_OutputPolyList();
            S_DumpScreen();
        }
    } else {
        while (InputStatus & IN_SELECT) {
            S_UpdateInput();
        }
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        T_DrawText();
        S_OutputPolyList();
        S_DumpScreen();
    }

    // wait till action key press
    while (!(InputStatus & IN_SELECT)) {
        if (IsResetFlag) {
            break;
        }
        S_InitialisePolyList();
        S_CopyBufferToScreen();
        S_UpdateInput();
        T_DrawText();
        S_OutputPolyList();
        S_DumpScreen();
    }

    // go to next level
    if (level_id == LEVEL10C) {
        SaveGame[0].bonus_flag = 1;
        for (int level = LEVEL1; level <= LEVEL10C; level++) {
            ModifyStartInfo(level);
        }
        SaveGame[0].current_level = 1;
    } else {
        CreateStartInfo(level_id + 1);
        ModifyStartInfo(level_id + 1);
        SaveGame[0].current_level = level_id + 1;
    }

    SaveGame[0].start[CURRENT].available = 0;
    S_FadeToBlack();
    TempVideoRemove();
}

int __cdecl S_LoadLevel(int level_id)
{
    TRACE("%d", level_id);
    int ret = LoadLevel(LevelNames[level_id], level_id);

    if (TR1MConfig.disable_healing_between_levels) {
        // check if we're in main menu by seeing if there is Lara item in the
        // currently loaded level.
        int lara_found = 0;
        for (int i = 0; i < LevelItemCount; i++) {
            if (Items[i].object_number == ID_LARA) {
                lara_found = 1;
            }
        }

        if (!lara_found) {
            TR1MData.stored_lara_health = LARA_HITPOINTS;
        }
    }

    return ret;
}

void __cdecl S_DrawHealthBar(int32_t percent)
{
    TR1MRenderBar(percent, 100, TRM1_BAR_LARA_HEALTH);
}

void __cdecl S_DrawAirBar(int32_t percent)
{
    TR1MRenderBar(percent, 100, TRM1_BAR_LARA_AIR);
}

int __cdecl LoadItems(FILE* handle)
{
    int32_t item_count = 0;
    _fread(&item_count, sizeof(int32_t), 1u, handle);

    TRACE("Item count: %d", item_count);

    if (item_count) {
        if (item_count > NUMBER_ITEMS) {
            strcpy(StringToShow, "LoadItems(): Too Many Items being Loaded!!");
            return 0;
        }

        Items = game_malloc(sizeof(ITEM_INFO) * NUMBER_ITEMS, GBUF_Items);
        if (!Items) {
            strcpy(
                StringToShow,
                "LoadItems(): Unable to allocate memory for 'items'");
            return 0;
        }

        LevelItemCount = item_count;
        InitialiseItemArray(NUMBER_ITEMS);

        for (int i = 0; i < item_count; ++i) {
            ITEM_INFO* item = &Items[i];
            _fread(&item->object_number, 2u, 1u, handle);
            _fread(&item->room_number, 2u, 1u, handle);
            _fread(&item->pos.x, 4u, 1u, handle);
            _fread(&item->pos.y, 4u, 1u, handle);
            _fread(&item->pos.z, 4u, 1u, handle);
            _fread(&item->pos.rot_y, 2u, 1u, handle);
            _fread(&item->shade, 2u, 1u, handle);
            _fread(&item->flags, 2u, 1u, handle);

            if (item->object_number < 0
                || item->object_number >= ID_NUMBER_OBJECTS) {
                sprintf(
                    StringToShow,
                    "LoadItems(): Bad Object number (%d) on Item %d",
                    item->object_number, i);
                S_ExitSystem(StringToShow);
            }

            if (TR1MConfig.disable_medpacks
                && (item->object_number == ID_LARGE_MEDIPACK_ITEM
                    || item->object_number == ID_SMALL_MEDIPACK_ITEM)) {
                item->pos.x = -1;
                item->pos.y = -1;
                item->pos.z = -1;
                item->room_number = 0;
            }

            InitialiseItem(i);
        }
    }

    return 1;
}

void __cdecl InitialiseLara()
{
    TRACE("");
    LaraItem->collidable = 0;
    LaraItem->data = &Lara;
    LaraItem->hit_points = TR1MConfig.disable_healing_between_levels
        ? TR1MData.stored_lara_health
        : LARA_HITPOINTS;

    Lara.air = LARA_AIR;
    Lara.torso_z_rot = 0;
    Lara.torso_x_rot = 0;
    Lara.torso_y_rot = 0;
    Lara.head_z_rot = 0;
    Lara.head_y_rot = 0;
    Lara.head_x_rot = 0;
    Lara.calc_fallspeed = 0;
    Lara.mesh_effects = 0;
    Lara.hit_frames = 0;
    Lara.hit_direction = 0;
    Lara.death_count = 0;
    Lara.target = 0;
    Lara.spaz_effect = 0;
    Lara.spaz_effect_count = 0;
    Lara.turn_rate = 0;
    Lara.move_angle = 0;
    Lara.right_arm.flash_gun = 0;
    Lara.left_arm.flash_gun = 0;
    Lara.right_arm.lock = 0;
    Lara.left_arm.lock = 0;

    if (RoomInfo[LaraItem->room_number].flags & 1) {
        Lara.water_status = LARA_UNDERWATER;
        LaraItem->fall_speed = 0;
        LaraItem->goal_anim_state = AS_TREAD;
        LaraItem->current_anim_state = AS_TREAD;
        LaraItem->anim_number = TREAD_A;
        LaraItem->frame_number = TREAD_F;
    } else {
        Lara.water_status = LARA_ABOVEWATER;
        LaraItem->goal_anim_state = AS_STOP;
        LaraItem->current_anim_state = AS_STOP;
        LaraItem->anim_number = STOP_A;
        LaraItem->frame_number = STOP_F;
    }

    Lara.current_active = 0;

    InitialiseLOT(&Lara.LOT);
    Lara.LOT.step = WALL_L * 20;
    Lara.LOT.drop = -WALL_L * 20;
    Lara.LOT.fly = STEP_L;

    InitialiseLaraInventory(CurrentLevel);
}

void __cdecl InitialiseFXArray()
{
    TRACE("");
    NextFxActive = NO_ITEM;
    NextFxFree = 0;
    FX_INFO* fx = Effects;
    for (int i = 1; i < NUM_EFFECTS; i++, fx++) {
        fx->next_fx = i;
    }
    fx->next_fx = NO_ITEM;
}

void __cdecl InitialiseLOTArray()
{
    TRACE("");
    BaddieSlots =
        game_malloc(NUM_SLOTS * sizeof(CREATURE_INFO), GBUF_CreatureData);
    CREATURE_INFO* creature = BaddieSlots;
    for (int i = 0; i < NUM_SLOTS; i++, creature++) {
        creature->item_num = NO_ITEM;
        creature->LOT.node =
            game_malloc(sizeof(BOX_NODE) * NumberBoxes, GBUF_CreatureLot);
    }
    SlotsUsed = 0;
}

void __cdecl DrawHealthBar()
{
    int hit_points = LaraItem->hit_points;
    if (hit_points < 0) {
        hit_points = 0;
    } else if (hit_points > LARA_HITPOINTS) {
        hit_points = LARA_HITPOINTS;
    }

    if (OldHitPoints != hit_points) {
        OldHitPoints = hit_points;
        HealthBarTimer = 40;
    }

    if (HealthBarTimer < 0) {
        HealthBarTimer = 0;
    }

    if (HealthBarTimer > 0 || hit_points <= 0 || Lara.gun_status == LG_READY) {
        S_DrawHealthBar(hit_points * 100 / LARA_HITPOINTS);
    }
}

void __cdecl DrawAirBar()
{
    if (Lara.water_status != LARA_UNDERWATER
        && Lara.water_status != LARA_SURFACE) {
        return;
    }

    int air = Lara.air;
    if (air < 0) {
        air = 0;
    } else if (Lara.air > LARA_AIR) {
        air = LARA_AIR;
    }

    S_DrawAirBar(air * 100 / LARA_AIR);
}

void __cdecl DrawPickups()
{
    int old_game_timer = OldGameTimer;
    OldGameTimer = SaveGame[0].timer;
    int16_t time = SaveGame[0].timer - old_game_timer;

    if (time > 0 && time < 60) {
        int y = PhdWinHeight - PhdWinWidth / 10;
        int x = PhdWinWidth - PhdWinWidth / 10;
        int sprite_width = 4 * (PhdWinWidth / 10) / 3;
        for (int i = 0; i < NUM_PU; i++) {
            DISPLAYPU* pu = &Pickups[i];
            pu->duration -= time;
            if (pu->duration <= 0) {
                pu->duration = 0;
            } else {
                S_DrawScreenSprite2d(
                    x, y, TR1MGetOverlayScale(12288), pu->sprnum, 4096);
                x -= sprite_width;
            }
        }
    }
}

void __cdecl DrawGameInfo()
{
    DrawAmmoInfo();
    if (OverlayStatus > 0) {
        DrawHealthBar();
        DrawAirBar();
        DrawPickups();

        if (TR1MConfig.enable_enemy_healthbar && Lara.target) {
            TR1MRenderBar(
                Lara.target->hit_points,
                Objects[Lara.target->object_number].hit_points,
                TRM1_BAR_ENEMY_HEALTH);
        }
    }

    T_DrawText();
}

void MakeAmmoString(char* string)
{
    char* c;

    for (c = string; *c != 0; c++) {
        if (*c == 32) {
            continue;
        } else if (*c - 'A' >= 0) {
            *c += 12 - 'A';
        } else {
            *c += 1 - '0';
        }
    }
}

void __cdecl DrawAmmoInfo()
{
    char ammostring[80] = "";

    if (Lara.gun_status != LG_READY || OverlayStatus <= 0
        || SaveGame[0].bonus_flag) {
        if (AmmoText) {
            T_RemovePrint(AmmoText);
            AmmoText = 0;
        }
        return;
    }

    switch (Lara.gun_type) {
    case LG_PISTOLS:
        return;
    case LG_MAGNUMS:
        sprintf(ammostring, "%5d B", Lara.magnums.ammo);
        break;
    case LG_UZIS:
        sprintf(ammostring, "%5d C", Lara.uzis.ammo);
        break;
    case LG_SHOTGUN:
        sprintf(ammostring, "%5d A", Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
        break;
    default:
        return;
    }

    MakeAmmoString(ammostring);

    if (AmmoText) {
        T_ChangeText(AmmoText, ammostring);
    } else {
        AmmoText = T_Print(-17, 22, 0, ammostring);
        T_RightAlign(AmmoText, 1);
    }
}
