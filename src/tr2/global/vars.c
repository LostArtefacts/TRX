#include "global/vars.h"

#include "game/spawn.h"

#include <libtrx/game/sound/ids.h>

#ifndef MESON_BUILD
const char *g_TRXVersion = "TR2X (non-Docker build)";
#endif

const float g_RhwFactor = 0x14000000.p0;

SDL_Window *g_SDLWindow = nullptr;

uint32_t g_PerspectiveDistance = 0x3000000;
uint32_t g_AssaultBestTime = -1;

int32_t g_OverlayStatus = 1;
bool g_GymInvOpenEnabled = true; // TODO: make me configurable
int32_t g_MidSort = 0;
int32_t g_PhdWinTop;
int32_t g_LsAdder;
float g_FltWinBottom;
float g_FltResZBuf;
float g_FltResZ;
int32_t g_PhdWinHeight;
int32_t g_PhdWinCenterX;
int32_t g_PhdWinCenterY;
float g_FltWinTop;
SORT_ITEM g_SortBuffer[4000];
float g_FltWinLeft;
int32_t g_PhdFarZ;
float g_FltRhwOPersp;
int32_t g_PhdWinBottom;
int32_t g_PhdPersp;
int32_t g_PhdWinLeft;
int16_t g_Info3DBuffer[120000];
int32_t g_PhdWinMaxX;
int32_t g_PhdNearZ;
float g_FltResZORhw;
float g_FltFarZ;
float g_FltWinCenterX;
float g_FltWinCenterY;
float g_FltPerspONearZ;
float g_FltRhwONearZ;
int32_t g_PhdWinMaxY;
float g_FltNearZ;
float g_FltPersp;
int16_t *g_Info3DPtr = nullptr;
int32_t g_PhdWinWidth;
int32_t g_PhdViewDistance;
int32_t g_LsDivider;
PHD_VBUF g_PhdVBuf[1500];
float g_FltWinRight;
XYZ_32 g_LsVectorView;
int32_t g_PhdWinRight;
int32_t g_SurfaceCount;
SORT_ITEM *g_Sort3DPtr = nullptr;
int32_t g_WibbleOffset;
bool g_IsWibbleEffect;
bool g_IsWaterEffect;
bool g_IsShadeEffect;
int32_t g_FlipTimer;
bool g_IsDemoLoaded;
bool g_IsAssaultTimerDisplay;
bool g_IsAssaultTimerActive;
bool g_IsMonkAngry;
uint16_t g_SoundOptionLine;
ASSAULT_STATS g_Assault;
int32_t g_HealthBarTimer;
int32_t g_LevelComplete;
SAVEGAME_INFO g_SaveGame = {};
LARA_INFO g_Lara;
ITEM *g_LaraItem = nullptr;
int32_t g_FlipStatus;
BOX_INFO *g_Boxes = nullptr;
int16_t *g_FlyZone[2] = {};
int16_t *g_GroundZone[4][2] = {};
uint16_t *g_Overlap = nullptr;
CREATURE *g_BaddieSlots = nullptr;

int16_t g_SampleLUT[SFX_NUMBER_OF];
SAMPLE_INFO *g_SampleInfos = nullptr;
int32_t g_HeightType;
int32_t g_FlipMaps[MAX_FLIP_MAPS];
bool g_CameraUnderwater;
int32_t g_BoxCount;
char g_LevelFileName[256];
uint16_t g_MusicTrackFlags[64];

WEAPON_INFO g_Weapons[] = {
    {},
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -60 * DEG_1, 60 * DEG_1 },
        .left_angles = { -170 * DEG_1, 60 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .right_angles = { -60 * DEG_1, 170 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 650,
        .damage = 1,
        .target_dist = 8192,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = 8,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -60 * DEG_1, 60 * DEG_1 },
        .left_angles = { -170 * DEG_1, 60 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .right_angles = { -60 * DEG_1, 170 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 650,
        .damage = 2,
        .target_dist = 8192,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = 21,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -60 * DEG_1, 60 * DEG_1 },
        .left_angles = { -170 * DEG_1, 60 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .right_angles = { -60 * DEG_1, 170 * DEG_1, -80 * DEG_1, 80 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 650,
        .damage = 1,
        .target_dist = 8192,
        .recoil_frame = 3,
        .flash_time = 3,
        .sample_num = 43,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 0,
        .gun_height = 500,
        .damage = 3,
        .target_dist = 8192,
        .recoil_frame = 9,
        .flash_time = 3,
        .sample_num = 45,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 728,
        .gun_height = 500,
        .damage = 3,
        .target_dist = 12288,
        .recoil_frame = 0,
        .flash_time = 3,
        .sample_num = 0,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 500,
        .damage = 30,
        .target_dist = 8192,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 0,
    },
    {
        .lock_angles = { -60 * DEG_1, 60 * DEG_1, -65 * DEG_1, 65 * DEG_1 },
        .left_angles = { -80 * DEG_1, 80 * DEG_1, -75 * DEG_1, 75 * DEG_1 },
        .right_angles = { -80 * DEG_1, 80 * DEG_1, -75 * DEG_1, 75 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 500,
        .damage = 4,
        .target_dist = 8192,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 0,
    },
    {},
    {
        .lock_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .left_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .right_angles = { -30 * DEG_1, 30 * DEG_1, -55 * DEG_1, 55 * DEG_1 },
        .aim_speed = 1820,
        .shot_accuracy = 1456,
        .gun_height = 400,
        .damage = 3,
        .target_dist = 8192,
        .recoil_frame = 0,
        .flash_time = 2,
        .sample_num = 43,
    },
};

int16_t g_FinalBossActive;
int16_t g_FinalLevelCount;
int16_t g_FinalBossCount;
int16_t g_FinalBossItem[5];

static char m_LoadGameRequesterStrings1[MAX_LEVELS][50];
static char m_LoadGameRequesterStrings2[MAX_LEVELS][50];

REQUEST_INFO g_LoadGameRequester = {
    .no_selector = 0,
    .ready = 0,
    .pad = 0,
    .items_count = 1,
    .selected = 0,
    .visible_count = 5,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 296,
    .line_height = 18,
    .x_pos = 0,
    .y_pos = -32,
    .z_pos = 0,
    .item_string_len = 50,
    .pitem_strings1 = (char *)m_LoadGameRequesterStrings1,
    .pitem_strings2 = (char *)m_LoadGameRequesterStrings2,
    .pitem_flags1 = nullptr,
    .pitem_flags2 = nullptr,
    .heading_flags1 = 0,
    .heading_flags2 = 0,
    .background_flags = 0,
    .moreup_flags = 0,
    .moredown_flags = 0,
    .item_flags1 = {},
    .item_flags2 = {},
    .heading_text1 = nullptr,
    .heading_text2 = nullptr,
    .background_text = nullptr,
    .moreup_text = nullptr,
    .moredown_text = nullptr,
    .item_texts1 = { nullptr },
    .item_texts2 = { nullptr },
    .heading_string1 = {},
    .heading_string2 = {},
    .render_width = 0,
    .render_height = 0,
};

REQUEST_INFO g_SaveGameRequester = {
    .no_selector = 0,
    .ready = 0,
    .pad = 0,
    .items_count = 1,
    .selected = 0,
    .visible_count = 5,
    .line_offset = 0,
    .line_old_offset = 0,
    .pix_width = 272,
    .line_height = 18,
    .x_pos = 0,
    .y_pos = -32,
    .z_pos = 0,
    .item_string_len = 50,
    .pitem_strings1 = (char *)g_ValidLevelStrings1,
    .pitem_strings2 = (char *)g_ValidLevelStrings2,
    .pitem_flags1 = nullptr,
    .pitem_flags2 = nullptr,
    .heading_flags1 = 0,
    .heading_flags2 = 0,
    .background_flags = 0,
    .moreup_flags = 0,
    .moredown_flags = 0,
    .item_flags1 = {},
    .item_flags2 = {},
    .heading_text1 = nullptr,
    .heading_text2 = nullptr,
    .background_text = nullptr,
    .moreup_text = nullptr,
    .moredown_text = nullptr,
    .item_texts1 = { nullptr },
    .item_texts2 = { nullptr },
    .heading_string1 = {},
    .heading_string2 = {},
    .render_width = 0,
    .render_height = 0,
};

bool g_GF_RemoveAmmo = false;
bool g_GF_RemoveWeapons = false;
bool g_GF_SunsetEnabled = false;
int16_t g_GF_NoFloor = 0;
int16_t g_GF_NumSecrets = 3;
int32_t g_GF_LaraStartAnim;
int32_t g_GF_ScriptVersion;

int32_t g_SavedGames;
TEXTSTRING *g_PasswordText1 = nullptr;
char g_ValidLevelStrings1[MAX_LEVELS][50];
char g_ValidLevelStrings2[MAX_LEVELS][50];
uint32_t g_RequesterFlags1[MAX_REQUESTER_ITEMS];
uint32_t g_RequesterFlags2[MAX_REQUESTER_ITEMS];
int32_t g_SaveCounter;
int16_t g_SavedLevels[MAX_LEVELS] = { -1, 0 };

XYZ_32 g_InteractPosition = { .x = 0, .y = 0, .z = 0 };
bool g_DetonateAllMines = false;
int32_t g_SunsetTimer = 0;
