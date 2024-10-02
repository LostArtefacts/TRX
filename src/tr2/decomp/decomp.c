#include "decomp/decomp.h"

#include "config.h"
#include "game/background.h"
#include "game/camera.h"
#include "game/console/common.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/gameflow/gameflow_new.h"
#include "game/hwr.h"
#include "game/input.h"
#include "game/inventory/backpack.h"
#include "game/inventory/common.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/draw.h"
#include "game/lot.h"
#include "game/math.h"
#include "game/music.h"
#include "game/overlay.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"
#include "lib/ddraw.h"
#include "lib/dinput.h"
#include "specific/s_flagged_string.h"

#include <libtrx/game/ui/common.h>
#include <libtrx/utils.h>

#include <assert.h>
#include <dinput.h>
#include <stdio.h>

#define IDI_MAINICON 100

static bool M_InsertDisplayModeInListSorted(
    DISPLAY_MODE_LIST *mode_list, DISPLAY_MODE *src_mode);

static void M_DisplayModeListInit(DISPLAY_MODE_LIST *mode_list);
static void M_DisplayModeListDelete(DISPLAY_MODE_LIST *mode_list);
static bool M_DisplayModeListCopy(
    DISPLAY_MODE_LIST *dst, DISPLAY_MODE_LIST *src);
static DISPLAY_MODE *M_InsertDisplayMode(
    DISPLAY_MODE_LIST *mode_list, DISPLAY_MODE_NODE *before);
static DISPLAY_MODE *M_InsertDisplayModeInListHead(
    DISPLAY_MODE_LIST *mode_list);
static DISPLAY_MODE *M_InsertDisplayModeInListTail(
    DISPLAY_MODE_LIST *mode_list);

static void M_DisplayModeListInit(DISPLAY_MODE_LIST *mode_list)
{
    mode_list->head = NULL;
    mode_list->tail = NULL;
    mode_list->count = 0;
}

static void M_DisplayModeListDelete(DISPLAY_MODE_LIST *mode_list)
{
    DISPLAY_MODE_NODE *node;
    DISPLAY_MODE_NODE *nextNode;

    for (node = mode_list->head; node; node = nextNode) {
        nextNode = node->next;
        free(node);
    }
    M_DisplayModeListInit(mode_list);
}

static bool M_DisplayModeListCopy(
    DISPLAY_MODE_LIST *dst, DISPLAY_MODE_LIST *src)
{
    if (dst == NULL || src == NULL || dst == src) {
        return false;
    }

    M_DisplayModeListDelete(dst);
    for (DISPLAY_MODE_NODE *node = src->head; node != NULL; node = node->next) {
        DISPLAY_MODE *dst_mode = M_InsertDisplayModeInListTail(dst);
        *dst_mode = node->body;
    }
    return true;
}

static DISPLAY_MODE *M_InsertDisplayMode(
    DISPLAY_MODE_LIST *mode_list, DISPLAY_MODE_NODE *before)
{
    if (!before || !before->previous) {
        return M_InsertDisplayModeInListHead(mode_list);
    }

    DISPLAY_MODE_NODE *node = malloc(sizeof(DISPLAY_MODE_NODE));
    if (!node) {
        return NULL;
    }

    before->previous->next = node;
    node->previous = before->previous;

    before->previous = node;
    node->next = before;

    mode_list->count++;
    return &node->body;
}

static DISPLAY_MODE *M_InsertDisplayModeInListHead(DISPLAY_MODE_LIST *mode_list)
{
    DISPLAY_MODE_NODE *node = malloc(sizeof(DISPLAY_MODE_NODE));
    if (!node) {
        return NULL;
    }

    node->next = mode_list->head;
    node->previous = NULL;

    if (mode_list->head) {
        mode_list->head->previous = node;
    }

    if (!mode_list->tail) {
        mode_list->tail = node;
    }

    mode_list->head = node;
    mode_list->count++;
    return &node->body;
}

static DISPLAY_MODE *M_InsertDisplayModeInListTail(DISPLAY_MODE_LIST *mode_list)
{
    DISPLAY_MODE_NODE *node = malloc(sizeof(DISPLAY_MODE_NODE));
    if (!node) {
        return NULL;
    }

    node->next = NULL;
    node->previous = mode_list->tail;

    if (mode_list->tail) {
        mode_list->tail->next = node;
    }

    if (!mode_list->head) {
        mode_list->head = node;
    }

    mode_list->tail = node;
    mode_list->count++;
    return &node->body;
}

static bool M_InsertDisplayModeInListSorted(
    DISPLAY_MODE_LIST *mode_list, DISPLAY_MODE *src_mode)
{
    DISPLAY_MODE *dst_mode = NULL;

    if (mode_list->head == NULL
        || CompareVideoModes(src_mode, &mode_list->head->body)) {
        dst_mode = M_InsertDisplayModeInListHead(mode_list);
        goto finish;
    }
    for (DISPLAY_MODE_NODE *node = mode_list->head; node != NULL;
         node = node->next) {
        if (CompareVideoModes(src_mode, &node->body)) {
            dst_mode = M_InsertDisplayMode(mode_list, node);
            goto finish;
        }
    }
    dst_mode = M_InsertDisplayModeInListTail(mode_list);

finish:
    if (dst_mode == NULL) {
        return false;
    }
    *dst_mode = *src_mode;
    return true;
}

int32_t __cdecl GameInit(void)
{
    Music_Shutdown();
    UT_InitAccurateTimer();
    // clang-format off
    Sound_Init();
    return WinVidInit()
        && Direct3DInit()
        && RenderInit()
        && InitTextures()
        && WinInputInit()
        && TIME_Init()
        && HWR_Init()
        && BGND_Init();
    // clang-format on
}

int32_t __stdcall WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine,
    int32_t nShowCmd)
{
    g_GameModule = hInstance;
    g_CmdLine = lpCmdLine;
    HWND game_window = WinVidFindGameWindow();
    if (game_window) {
        HWND setup_window = SE_FindSetupDialog();
        if (!setup_window) {
            setup_window = game_window;
        }
        SetForegroundWindow(setup_window);
        return 0;
    }

    // TODO: install exception handler

    bool is_setup_requested = UT_FindArg("setup") != 0;
    if (!GameInit()) {
        UT_ErrorBox(IDS_DX5_REQUIRED, NULL);
        goto cleanup;
    }

    int32_t app_settings_status = SE_ReadAppSettings(&g_SavedAppSettings);
    if (!app_settings_status) {
        goto cleanup;
    }

    if (app_settings_status == 2 || is_setup_requested) {
        if (SE_ShowSetupDialog(0, app_settings_status == 2)) {
            SE_WriteAppSettings(&g_SavedAppSettings);
            if (is_setup_requested) {
                goto cleanup;
            }
        } else {
            goto cleanup;
        }
    }

    int32_t result = WinGameStart();
    if (result) {
        Shell_Shutdown();
        RenderErrorBox(result);
        if (!SE_ShowSetupDialog(0, 0)) {
            goto cleanup;
        }
        SE_WriteAppSettings(&g_SavedAppSettings);
    } else {
        g_StopInventory = 0;
        g_IsGameToExit = 0;
        Shell_Main();
        Config_Write();
        Shell_Shutdown();
        SE_WriteAppSettings(&g_SavedAppSettings);
    }

cleanup:
    Shell_Cleanup();
    return g_AppResultCode;
}

const char *__cdecl DecodeErrorMessage(int32_t error_code)
{
    return g_ErrorMessages[error_code];
}

int32_t __cdecl RenderErrorBox(int32_t error_code)
{
    char buffer[128];
    const char *decoded = DecodeErrorMessage(error_code);
    sprintf(buffer, "Render init failed with \"%s\"", decoded);
    return UT_MessageBox(buffer, 0);
}

void __cdecl ScreenshotPCX(void)
{
    LPDDS screen = g_SavedAppSettings.render_mode == RM_SOFTWARE
        ? g_RenderBufferSurface
        : g_PrimaryBufferSurface;

    DDSURFACEDESC desc = { .dwSize = sizeof(DDSURFACEDESC), 0 };

    int32_t result;
    while (true) {
        result = IDirectDrawSurface_Lock(screen, 0, &desc, 0, 0);
        if (result != DDERR_WASSTILLDRAWING) {
            break;
        }
    }

    if (result == DDERR_SURFACELOST) {
        IDirectDrawSurface_Restore(screen);
    }

    if (FAILED(result)) {
        return;
    }

    uint8_t *pcx_data;
    int32_t pcx_size = CompPCX(
        desc.lpSurface, desc.dwWidth, desc.dwHeight, g_GamePalette8, &pcx_data);

    IDirectDrawSurface_Unlock(screen, &desc);
    if (!pcx_size) {
        return;
    }

    g_ScreenshotCounter++;
    if (g_ScreenshotCounter > 9999) {
        g_ScreenshotCounter = 1;
    }

    char file_name[20];
    sprintf(file_name, "tomb%04d.pcx", g_ScreenshotCounter);

    HANDLE handle = CreateFileA(
        file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
        NULL);
    DWORD bytes_written;
    WriteFile(handle, pcx_data, pcx_size, &bytes_written, 0);
    CloseHandle(handle);
    GlobalFree(pcx_data);
}

size_t __cdecl CompPCX(
    uint8_t *bitmap, int32_t width, int32_t height, RGB_888 *palette,
    uint8_t **pcx_data)
{
    *pcx_data = (uint8_t *)GlobalAlloc(
        GMEM_FIXED,
        sizeof(PCX_HEADER) + sizeof(RGB_888) * 256 + width * height * 2);
    if (*pcx_data == NULL) {
        return 0;
    }

    PCX_HEADER *pcx_header = (PCX_HEADER *)*pcx_data;
    pcx_header->manufacturer = 10;
    pcx_header->version = 5;
    pcx_header->rle = 1;
    pcx_header->bpp = 8;
    pcx_header->planes = 1;
    pcx_header->x_min = 0;
    pcx_header->y_min = 0;
    pcx_header->x_max = width - 1;
    pcx_header->y_max = height - 1;
    pcx_header->h_dpi = width;
    pcx_header->v_dpi = height;
    pcx_header->bytes_per_line = width;

    uint8_t *pic_data = *pcx_data + sizeof(PCX_HEADER);
    for (int32_t y = 0; y < height; y++) {
        pic_data += EncodeLinePCX(bitmap, width, pic_data);
        bitmap += width;
    }

    *pic_data++ = 0x0C;
    for (int32_t i = 0; i < 256; i++) {
        *pic_data++ = palette[i].red;
        *pic_data++ = palette[i].green;
        *pic_data++ = palette[i].blue;
    }

    return pic_data - *pcx_data + sizeof(RGB_888) * 256;
}

size_t __cdecl EncodeLinePCX(
    const uint8_t *src, const int32_t width, uint8_t *dst)
{
    const uint8_t *const dst_start = dst;
    int32_t run_count = 1;
    uint8_t last = *src;

    for (int32_t i = 1; i < width; i++) {
        uint8_t current = *src++;
        if (*src == last) {
            run_count++;
            if (run_count == 63) {
                const size_t add = EncodePutPCX(last, 0x3Fu, dst);
                if (add == 0) {
                    return 0;
                }
                dst += add;
                run_count = 0;
            }
        } else {
            if (run_count != 0) {
                const size_t add = EncodePutPCX(last, run_count, dst);
                if (add == 0) {
                    return 0;
                }
                dst += add;
            }
            last = current;
            run_count = 1;
        }
    }

    if (run_count != 0) {
        const size_t add = EncodePutPCX(last, run_count, dst);
        if (add == 0) {
            return 0;
        }
        dst += add;
    }

    const size_t total = dst - dst_start;
    return total;
}

size_t __cdecl EncodePutPCX(uint8_t value, uint8_t num, uint8_t *buffer)
{
    if (num == 0 || num > 63) {
        return 0;
    }

    if (num == 1 && (value & 0xC0) != 0xC0) {
        buffer[0] = value;
        return 1;
    }

    buffer[0] = num | 0xC0;
    buffer[1] = value;
    return 2;
}

void __cdecl ScreenshotTGA(IDirectDrawSurface3 *screen, int32_t bpp)
{
    DDSURFACEDESC desc = {
        .dwSize = sizeof(DDSURFACEDESC),
        0,
    };

    if (FAILED(WinVidBufferLock(screen, &desc, 0x21u))) {
        return;
    }

    const int32_t width = desc.dwWidth;
    const int32_t height = desc.dwHeight;

    g_ScreenshotCounter++;

    char file_name[20];
    sprintf(file_name, "tomb%04d.tga", g_ScreenshotCounter);

    FILE *handle = fopen(file_name, "wb");
    if (!handle) {
        return;
    }

    const TGA_HEADER header = {
        .id_length = 0,
        .color_map_type = 0,
        .data_type_code = 2, // Uncompressed, RGB images
        .color_map_origin = 0,
        .color_map_length = 0,
        .color_map_depth = 0,
        .x_origin = 0,
        .y_origin = 0,
        .width = width,
        .height = height,
        .bpp = 16,
        .image_descriptor = 0,
    };

    fwrite(&header, sizeof(TGA_HEADER), 1, handle);

    uint8_t *tga_pic =
        (uint8_t *)GlobalAlloc(GMEM_FIXED, width * height * (bpp / 8));
    uint8_t *src = desc.lpSurface + desc.lPitch * (height - 1);

    uint8_t *dst = tga_pic;
    for (int32_t y = 0; y < height; y++) {
        if (desc.ddpfPixelFormat.dwRBitMask == 0xF800) {
            // R5G6B5 - transform
            for (int32_t x = 0; x < width; x++) {
                const uint16_t sample = ((uint16_t *)src)[x];
                ((uint16_t *)dst)[x] =
                    ((sample & 0xFFC0) >> 1) | (sample & 0x001F);
            }
        } else {
            // X1R5G5B5 - good
            memcpy(dst, src, sizeof(uint16_t) * width);
        }
        src -= desc.lPitch;
        dst += sizeof(uint16_t) * width;
    }
    fwrite(tga_pic, 2 * height * width, 1, handle);

cleanup:
    if (tga_pic) {
        GlobalFree(tga_pic);
    }

    if (handle) {
        fclose(handle);
    }
    WinVidBufferUnlock(screen, &desc);
}

void __cdecl Screenshot(LPDDS screen)
{
    DDSURFACEDESC desc = { 0 };
    desc.dwSize = sizeof(DDSURFACEDESC);

    if (SUCCEEDED(IDirectDrawSurface_GetSurfaceDesc(screen, &desc))) {
        if (desc.ddpfPixelFormat.dwRGBBitCount == 8) {
            ScreenshotPCX();
        } else if (desc.ddpfPixelFormat.dwRGBBitCount == 16) {
            ScreenshotTGA(screen, 16);
        }
    }
}

bool __cdecl DInputCreate(void)
{
    return SUCCEEDED(DirectInputCreate(g_GameModule, 1280, &g_DInput, NULL));
}

void __cdecl DInputRelease(void)
{
    if (g_DInput) {
        IDirectInput_Release(g_DInput);
        g_DInput = NULL;
    }
}

void __cdecl WinInReadKeyboard(uint8_t *input_data)
{
    if (SUCCEEDED(IDirectInputDevice_GetDeviceState(
            IDID_SysKeyboard, 256, input_data))) {
        return;
    }

    if (SUCCEEDED(IDirectInputDevice_Acquire(IDID_SysKeyboard))
        && SUCCEEDED(IDirectInputDevice_GetDeviceState(
            IDID_SysKeyboard, 256, input_data))) {
        return;
    }

    memset(input_data, 0, 256);
}

int32_t __cdecl WinGameStart(void)
{
    // try {
    WinVidStart();
    RenderStart(1);
    WinInStart();
    // } catch (int32_t error) {
    //     return error;
    // }
    return 0;
}

void __cdecl Shell_Shutdown(void)
{
    Console_Shutdown();
    WinInFinish();
    RenderFinish(1);
    WinVidFinish();
    WinVidHideGameWindow();
    if (g_ErrorMessage[0]) {
        MessageBoxA(NULL, g_ErrorMessage, NULL, MB_ICONWARNING);
    }
    Text_Shutdown();
    UI_Shutdown();
    Config_Shutdown();
}

int16_t __cdecl TitleSequence(void)
{
    GF_N_LoadStrings(-1);

    TempVideoAdjust(1, 1.0);
    g_NoInputCounter = 0;

    if (!g_IsTitleLoaded) {
        if (!Level_Initialise(0, GFL_TITLE)) {
            return GFD_EXIT_GAME;
        }
        g_IsTitleLoaded = true;
    }

    S_DisplayPicture("data/title.pcx", true);
    if (g_GameFlow.title_track) {
        Music_Play(g_GameFlow.title_track, true);
    }

    GAME_FLOW_DIR dir = Inv_Display(INV_TITLE_MODE);

    S_FadeToBlack();
    S_DontDisplayPicture();
    Music_Stop();

    if (dir == GFD_OVERRIDE) {
        dir = g_GF_OverrideDir;
        g_GF_OverrideDir = (GAME_FLOW_DIR)-1;
        return dir;
    }

    if (dir == GFD_START_DEMO) {
        return GFD_START_DEMO;
    }

    if (g_Inv_Chosen == O_PHOTO_OPTION) {
        return GFD_START_GAME | LV_GYM;
    }

    if (g_Inv_Chosen == O_PASSPORT_OPTION) {
        const int32_t slot_num = g_Inv_ExtraData[1];

        if (g_Inv_ExtraData[0] == 0) {
            Inv_RemoveAllItems();
            S_LoadGame(&g_SaveGame, sizeof(SAVEGAME_INFO), slot_num);
            return GFD_START_SAVED_GAME | slot_num;
        }

        if (g_Inv_ExtraData[0] == 1) {
            InitialiseStartInfo();
            int32_t level_id = LV_FIRST;
            if (g_GameFlow.play_any_level) {
                level_id = LV_FIRST + slot_num;
            }
            return GFD_START_GAME | level_id;
        }
        return GFD_EXIT_GAME;
    }

    return GFD_EXIT_GAME;
}

void __cdecl WinVidSetMinWindowSize(int32_t width, int32_t height)
{
    g_MinWindowClientWidth = width;
    g_MinWindowClientHeight = height;
    GameWindowCalculateSizeFromClient(&width, &height);
    g_MinWindowWidth = width;
    g_MinWindowHeight = height;
    g_IsMinWindowSizeSet = true;
}

void __cdecl WinVidSetMaxWindowSize(int32_t width, int32_t height)
{
    g_MaxWindowClientWidth = width;
    g_MaxWindowClientHeight = height;
    GameWindowCalculateSizeFromClient(&width, &height);
    g_MaxWindowWidth = width;
    g_MaxWindowHeight = height;
    g_IsMaxWindowSizeSet = true;
}

void __cdecl WinVidClearMinWindowSize(void)
{
    g_IsMinWindowSizeSet = false;
}

void __cdecl WinVidClearMaxWindowSize(void)
{
    g_IsMaxWindowSizeSet = false;
}

int32_t __cdecl CalculateWindowWidth(const int32_t width, const int32_t height)
{
    if (g_SavedAppSettings.aspect_mode == AM_4_3) {
        return 4 * height / 3;
    }
    if (g_SavedAppSettings.aspect_mode == AM_16_9) {
        return 16 * height / 9;
    }
    return width;
}

int32_t __cdecl CalculateWindowHeight(const int32_t width, const int32_t height)
{
    if (g_SavedAppSettings.aspect_mode == AM_4_3) {
        return (3 * width) / 4;
    }
    if (g_SavedAppSettings.aspect_mode == AM_16_9) {
        return (9 * width) / 16;
    }
    return height;
}

bool __cdecl WinVidGetMinMaxInfo(LPMINMAXINFO info)
{
    if (!g_IsGameWindowCreated) {
        return false;
    }

    if (g_IsGameFullScreen) {
        info->ptMaxTrackSize.x = g_FullScreenWidth;
        info->ptMaxTrackSize.y = g_FullScreenHeight;
        info->ptMinTrackSize.x = g_FullScreenWidth;
        info->ptMinTrackSize.y = g_FullScreenHeight;
        info->ptMaxSize.x = g_FullScreenWidth;
        info->ptMaxSize.y = g_FullScreenHeight;
        return true;
    }

    if (g_IsMinWindowSizeSet) {
        info->ptMinTrackSize.x = g_MinWindowWidth;
        info->ptMinTrackSize.y = g_MinWindowHeight;
    }

    if (g_IsMinMaxInfoSpecial) {
        int32_t width = g_GameWindowWidth;
        int32_t height = g_GameWindowHeight;
        GameWindowCalculateSizeFromClient(&width, &height);
        info->ptMaxSize.x = width;
        info->ptMaxTrackSize.x = width;
        info->ptMaxSize.y = height;
        info->ptMaxTrackSize.y = height;
    } else if (g_IsMaxWindowSizeSet) {
        info->ptMaxTrackSize.x = g_MaxWindowWidth;
        info->ptMaxTrackSize.y = g_MaxWindowHeight;
        info->ptMaxSize.x = g_MaxWindowWidth;
        info->ptMaxSize.y = g_MaxWindowHeight;
    }

    return g_IsMinWindowSizeSet || g_IsMaxWindowSizeSet;
}

HWND __cdecl WinVidFindGameWindow(void)
{
    return FindWindowA(CLASS_NAME, WINDOW_NAME);
}

bool __cdecl WinVidSpinMessageLoop(bool need_wait)
{
    if (g_IsMessageLoopClosed) {
        return 0;
    }

    g_MessageLoopCounter++;

    do {
        if (need_wait) {
            WaitMessage();
        } else {
            need_wait = true;
        }

        MSG msg;
        while (PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            if (msg.message == WM_QUIT) {
                g_AppResultCode = msg.wParam;
                g_IsMessageLoopClosed = true;
                g_IsGameToExit = true;
                g_StopInventory = true;
                g_MessageLoopCounter--;
                return 0;
            } else if (msg.message == WM_KEYDOWN) {
                UI_HandleKeyDown(msg.wParam);
                return 0;
            } else if (msg.message == WM_KEYUP) {
                UI_HandleKeyUp(msg.wParam);
                return 0;
            } else if (msg.message == WM_CHAR) {
                char insert_string[2] = { msg.wParam, '\0' };
                UI_HandleTextEdit(insert_string);
                return 0;
            }
        }
    } while (!g_IsGameWindowActive || g_IsGameWindowMinimized);

    g_MessageLoopCounter--;
    return true;
}

void __cdecl WinVidShowGameWindow(const int32_t cmd_show)
{
    if (cmd_show != SW_SHOW || !g_IsGameWindowShow) {
        g_IsGameWindowUpdating = true;
        ShowWindow(g_GameWindowHandle, cmd_show);
        UpdateWindow(g_GameWindowHandle);
        g_IsGameWindowUpdating = false;
        g_IsGameWindowShow = true;
    }
}

void __cdecl WinVidHideGameWindow(void)
{
    if (g_IsGameWindowShow) {
        g_IsGameWindowUpdating = true;
        ShowWindow(g_GameWindowHandle, SW_HIDE);
        UpdateWindow(g_GameWindowHandle);
        g_IsGameWindowUpdating = false;
        g_IsGameWindowShow = false;
    }
}

void __cdecl WinVidSetGameWindowSize(int32_t width, int32_t height)
{
    GameWindowCalculateSizeFromClient(&width, &height);
    SetWindowPos(
        g_GameWindowHandle, NULL, 0, 0, width, height,
        SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
}

bool __cdecl ShowDDrawGameWindow(bool active)
{
    if (!g_GameWindowHandle || !g_DDraw) {
        return false;
    }
    if (g_IsDDrawGameWindowShow) {
        return true;
    }

    RECT rect;
    GetWindowRect(g_GameWindowHandle, &rect);
    g_GameWindowX = rect.left;
    g_GameWindowY = rect.top;

    if (active) {
        WinVidShowGameWindow(SW_SHOW);
    }

    g_IsGameWindowUpdating = true;
    uint32_t flags = DDSCL_ALLOWMODEX | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT
        | DDSCL_FULLSCREEN;
    if (!active)
        flags |= DDSCL_NOWINDOWCHANGES;
    const HRESULT result =
        IDirectDraw_SetCooperativeLevel(g_DDraw, g_GameWindowHandle, flags);
    g_IsGameWindowUpdating = false;
    if (FAILED(result)) {
        return false;
    }

    g_IsDDrawGameWindowShow = true;
    return true;
}

bool __cdecl HideDDrawGameWindow(void)
{
    if (!g_GameWindowHandle || !g_DDraw) {
        return false;
    }
    if (!g_IsDDrawGameWindowShow) {
        return true;
    }

    WinVidHideGameWindow();
    g_IsGameWindowUpdating = true;
    const HRESULT result = IDirectDraw_SetCooperativeLevel(
        g_DDraw, g_GameWindowHandle, DDSCL_NORMAL);
    if (SUCCEEDED(result)) {
        g_IsDDrawGameWindowShow = false;
        SetWindowPos(
            g_GameWindowHandle, NULL, g_GameWindowX, g_GameWindowY, 0, 0,
            SWP_NOCOPYBITS | SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
    }

    g_IsGameWindowUpdating = false;
    return SUCCEEDED(result);
}

HRESULT __cdecl DDrawSurfaceCreate(LPDDSDESC dsp, LPDDS *surface)
{
    LPDIRECTDRAWSURFACE sub_surface;
    HRESULT rc = IDirectDraw_CreateSurface(g_DDraw, dsp, &sub_surface, NULL);

    if SUCCEEDED (rc) {
        rc = IDirectDrawSurface_QueryInterface(
            sub_surface, &g_IID_IDirectDrawSurface3, (LPVOID *)surface);
        IDirectDrawSurface_Release(sub_surface);
    }

    return rc;
}

HRESULT __cdecl DDrawSurfaceRestoreLost(
    LPDDS surface1, LPDDS surface2, bool blank)
{
    HRESULT rc = IDirectDrawSurface_IsLost(surface1);
    if (rc != DDERR_SURFACELOST) {
        return rc;
    }
    rc = IDirectDrawSurface_Restore(surface2 != NULL ? surface2 : surface1);
    if (blank && SUCCEEDED(rc)) {
        WinVidClearBuffer(surface1, 0, 0);
    }
    return rc;
}

bool __cdecl WinVidClearBuffer(LPDDS surface, LPRECT rect, DWORD fill_color)
{
    DDBLTFX blt = { .dwFillColor = fill_color, .dwSize = sizeof(DDBLTFX), 0 };
    HRESULT rc = IDirectDrawSurface_Blt(
        surface, rect, NULL, NULL, DDBLT_WAIT | DDBLT_COLORFILL, &blt);
    return SUCCEEDED(rc);
}

HRESULT __cdecl WinVidBufferLock(LPDDS surface, LPDDSDESC desc, DWORD flags)
{
    memset(desc, 0, sizeof(DDSURFACEDESC));
    desc->dwSize = sizeof(DDSURFACEDESC);
    HRESULT rc = IDirectDrawSurface_Lock(surface, NULL, desc, flags, NULL);
    if (SUCCEEDED(rc)) {
        g_LockedBufferCount++;
    }
    return rc;
}

HRESULT __cdecl WinVidBufferUnlock(LPDDS surface, LPDDSDESC desc)
{
    HRESULT rc = surface->lpVtbl->Unlock(surface, desc->lpSurface);
    if (SUCCEEDED(rc)) {
        g_LockedBufferCount--;
    }
    return rc;
}

bool __cdecl WinVidCopyBitmapToBuffer(LPDDS surface, const BYTE *bitmap)
{
    DDSURFACEDESC desc;
    if (FAILED(
            WinVidBufferLock(surface, &desc, DDLOCK_WRITEONLY | DDLOCK_WAIT))) {
        return false;
    }

    const uint8_t *src = (const uint8_t *)bitmap;
    uint8_t *dst = (uint8_t *)desc.lpSurface;
    for (int32_t i = 0; i < (int32_t)desc.dwHeight; i++) {
        memcpy(dst, src, desc.dwWidth);
        src += desc.dwWidth;
        dst += desc.lPitch;
    }
    WinVidBufferUnlock(surface, &desc);
    return true;
}

DWORD __cdecl GetRenderBitDepth(const uint32_t rgb_bit_count)
{
    switch (rgb_bit_count) {
        // clang-format off
        case 1:    return DDBD_1;
        case 2:    return DDBD_2;
        case 4:    return DDBD_4;
        case 8:    return DDBD_8;
        case 0x10: return DDBD_16;
        case 0x18: return DDBD_24;
        case 0x20: return DDBD_32;
        // clang-format on
    }
    return 0;
}

void __thiscall WinVidGetColorBitMasks(
    COLOR_BIT_MASKS *bm, LPDDPIXELFORMAT pixel_format)
{
    bm->mask.r = pixel_format->dwRBitMask;
    bm->mask.g = pixel_format->dwGBitMask;
    bm->mask.b = pixel_format->dwBBitMask;
    bm->mask.a = pixel_format->dwRGBAlphaBitMask;
    BitMaskGetNumberOfBits(bm->mask.r, &bm->depth.r, &bm->offset.r);
    BitMaskGetNumberOfBits(bm->mask.g, &bm->depth.g, &bm->offset.g);
    BitMaskGetNumberOfBits(bm->mask.b, &bm->depth.b, &bm->offset.b);
    BitMaskGetNumberOfBits(bm->mask.a, &bm->depth.a, &bm->offset.a);
}

void __cdecl BitMaskGetNumberOfBits(
    uint32_t bit_mask, uint32_t *bit_depth, uint32_t *bit_offset)
{
    if (!bit_mask) {
        *bit_offset = 0;
        *bit_depth = 0;
        return;
    }

    int32_t i;

    for (i = 0; (bit_mask & 1) == 0; i++) {
        bit_mask >>= 1;
    }
    *bit_offset = i;

    for (i = 0; bit_mask != 0; i++) {
        bit_mask >>= 1;
    }
    *bit_depth = i;
}

DWORD __cdecl CalculateCompatibleColor(
    const COLOR_BIT_MASKS *const mask, const int32_t red, const int32_t green,
    const int32_t blue, const int32_t alpha)
{
    // clang-format off
    return (
        (red   >> (8 - mask->depth.r) << mask->offset.r) |
        (green >> (8 - mask->depth.g) << mask->offset.g) |
        (blue  >> (8 - mask->depth.b) << mask->offset.b) |
        (alpha >> (8 - mask->depth.a) << mask->offset.a)
    );
    // clang-format on
}

bool __cdecl WinVidGetDisplayMode(DISPLAY_MODE *disp_mode)
{
    DDSDESC dsp = { .dwSize = sizeof(DDSDESC), 0 };

    if (FAILED(IDirectDraw_GetDisplayMode(g_DDraw, &dsp))) {
        return false;
    }

    // clang-format off
    if (!(dsp.dwFlags & DDSD_WIDTH)
        || !(dsp.dwFlags & DDSD_HEIGHT)
        || !(dsp.dwFlags & DDSD_PIXELFORMAT)
        || !(dsp.ddpfPixelFormat.dwFlags & DDPF_RGB)
    ) {
        return false;
    }
    // clang-format on

    disp_mode->width = dsp.dwWidth;
    disp_mode->height = dsp.dwHeight;
    disp_mode->bpp = dsp.ddpfPixelFormat.dwRGBBitCount;
    disp_mode->vga = (dsp.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) != 0
        ? VGA_256_COLOR
        : VGA_NO_VGA;
    return true;
}

bool __cdecl WinVidGoFullScreen(DISPLAY_MODE *disp_mode)
{
    g_FullScreenWidth = disp_mode->width;
    g_FullScreenHeight = disp_mode->height;
    g_FullScreenBPP = disp_mode->bpp;
    g_FullScreenVGA = disp_mode->vga;

    if (!ShowDDrawGameWindow(true)) {
        return false;
    }

    g_IsGameWindowUpdating = true;
    const HRESULT rc = IDirectDraw4_SetDisplayMode(
        g_DDraw, disp_mode->width, disp_mode->height, disp_mode->bpp, 0,
        disp_mode->vga == VGA_STANDARD ? DDSDM_STANDARDVGAMODE : 0);
    g_IsGameWindowUpdating = false;

    if (FAILED(rc)) {
        return false;
    }

    g_IsGameFullScreen = true;
    return true;
}

bool __cdecl WinVidGoWindowed(
    int32_t width, int32_t height, DISPLAY_MODE *disp_mode)
{
    if (!HideDDrawGameWindow()) {
        return false;
    }
    if (!WinVidGetDisplayMode(disp_mode)) {
        return false;
    }

    int32_t max_width = disp_mode->width;
    int32_t max_height =
        CalculateWindowHeight(disp_mode->width, disp_mode->height);
    if (max_height > disp_mode->height) {
        max_height = disp_mode->height;
        max_width = CalculateWindowWidth(disp_mode->width, max_height);
    }
    WinVidSetMaxWindowSize(max_width, max_height);

    if (width > max_width || height > max_height) {
        width = max_width;
        height = max_height;
    }

    g_IsGameFullScreen = false;
    g_IsGameWindowUpdating = true;
    WinVidSetGameWindowSize(width, height);
    g_IsGameWindowUpdating = false;

    RECT rect;
    GetClientRect(g_GameWindowHandle, &rect);
    MapWindowPoints(g_GameWindowHandle, NULL, (LPPOINT)&rect, 2);

    if ((rect.left > 0 || rect.right < disp_mode->width)
        && (rect.top > 0 || rect.bottom < disp_mode->height)) {
        WinVidShowGameWindow(SW_SHOW);
    } else {
        WinVidShowGameWindow(SW_MAXIMIZE);
    }

    disp_mode->width = width;
    disp_mode->height = height;
    return true;
}

void __cdecl WinVidSetDisplayAdapter(DISPLAY_ADAPTER *disp_adapter)
{
    DISPLAY_MODE disp_mode;

    disp_adapter->sw_windowed_supported = false;
    disp_adapter->hw_windowed_supported = false;
    disp_adapter->screen_width = 0;

    if (disp_adapter->adapter_guid_ptr != NULL) {
        return;
    }

    if (!DDrawCreate(NULL)) {
        return;
    }

    bool result = WinVidGetDisplayMode(&disp_mode);
    DDrawRelease();

    if (!result) {
        return;
    }

    disp_mode.width &= ~0x1F;
    if (disp_mode.width * 3 / 4 > disp_mode.height) {
        disp_mode.width = (disp_mode.height * 4 / 3) & ~0x1F;
    }

    disp_adapter->sw_windowed_supported = disp_mode.vga == VGA_256_COLOR;
    disp_adapter->hw_windowed_supported = disp_adapter->hw_render_supported
        && (disp_adapter->hw_device_desc.dwFlags & D3DDD_DEVICERENDERBITDEPTH)
            != 0
        && (GetRenderBitDepth(disp_mode.bpp)
            & disp_adapter->hw_device_desc.dwDeviceRenderBitDepth)
            != 0;
}

void __cdecl Game_SetCutsceneTrack(const int32_t track)
{
    g_CineTrackID = track;
}

int32_t __cdecl Game_Cutscene_Start(const int32_t level_num)
{
    g_IsTitleLoaded = false;
    S_FadeToBlack();
    if (!Level_Initialise(level_num, GFL_CUTSCENE)) {
        return 2;
    }

    Misc_InitCinematicRooms();
    CutscenePlayer1_Initialise(g_Lara.item_num);
    g_Camera.target_angle = g_CineTargetAngle;

    const bool old_sound_active = g_SoundIsActive;
    g_SoundIsActive = false;

    g_CineFrameIdx = 0;
    S_ClearScreen();

    if (!Music_PlaySynced(g_CineTrackID)) {
        return 1;
    }

    Music_SetVolume(255);
    g_CineFrameCurrent = 0;

    int32_t result;
    do {
        Game_DrawCinematic();
        int32_t nticks =
            g_CineFrameCurrent - TICKS_PER_FRAME * (g_CineFrameIdx - 4);
        CLAMPL(nticks, TICKS_PER_FRAME);
        result = Game_Cutscene_Control(nticks);
    } while (!result);

    if (g_OptionMusicVolume) {
        Music_SetVolume(25 * g_OptionMusicVolume + 5);
    } else {
        Music_SetVolume(0);
    }
    Music_Stop();
    g_SoundIsActive = old_sound_active;
    Sound_StopAllSamples();

    g_LevelComplete = true;
    return result;
}

void __cdecl Misc_InitCinematicRooms(void)
{
    for (int32_t i = 0; i < g_RoomCount; i++) {
        const int16_t flipped_room = g_Rooms[i].flipped_room;
        if (flipped_room != NO_ROOM_NEG) {
            g_Rooms[flipped_room].bound_active = 1;
        }
        g_Rooms[i].flags |= RF_OUTSIDE;
    }

    g_DrawRoomsCount = 0;
    for (int32_t i = 0; i < g_RoomCount; i++) {
        if (!g_Rooms[i].bound_active) {
            g_DrawRoomsArray[g_DrawRoomsCount++] = i;
        }
    }
}

int32_t __cdecl Game_Cutscene_Control(const int32_t nframes)
{
    g_CineTickCount += g_CineTickRate * nframes;

    if (g_CineTickCount >= 0) {
        while (1) {
            if (g_GF_OverrideDir != (GAME_FLOW_DIR)-1) {
                return 4;
            }

            if (Input_Update()) {
                return 3;
            }
            if (g_InputDB & IN_ACTION) {
                return 1;
            }
            if (g_InputDB & IN_OPTION) {
                return 2;
            }

            g_DynamicLightCount = 0;

            for (int32_t id = g_NextItemActive; id != NO_ITEM;) {
                const ITEM *const item = &g_Items[id];
                const OBJECT *obj = &g_Objects[item->object_id];
                if (obj->control != NULL) {
                    obj->control(id);
                }
                id = item->next_active;
            }

            for (int32_t id = g_NextEffectActive; id != NO_ITEM;) {
                const FX *const fx = &g_Effects[id];
                const OBJECT *const obj = &g_Objects[fx->object_id];
                if (obj->control != NULL) {
                    obj->control(id);
                }
                id = fx->next_active;
            }

            HairControl(1);
            Camera_UpdateCutscene();

            g_CineFrameIdx++;
            if (g_CineFrameIdx >= g_NumCineFrames) {
                return 1;
            }

            g_CineTickCount -= 0x10000;
            if (g_CineTickCount < 0) {
                break;
            }
        }
    }

    if (Music_GetTimestamp() < 0.0) {
        g_CineFrameCurrent++;
    } else {
        // sync with music
        g_CineFrameCurrent =
            Music_GetTimestamp() * FRAMES_PER_SECOND * TICKS_PER_FRAME / 1000.0;
    }

    return 0;
}

void __cdecl CutscenePlayer_Control(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    item->rot.y = g_Camera.target_angle;
    item->pos.x = g_Camera.pos.pos.x;
    item->pos.y = g_Camera.pos.pos.y;
    item->pos.z = g_Camera.pos.pos.z;

    XYZ_32 pos = { 0 };
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_FindByPos(pos.x, pos.y, pos.z);
    if (room_num != NO_ROOM_NEG && item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->dynamic_light && item->status != IS_INVISIBLE) {
        pos.x = 0;
        pos.y = 0;
        pos.z = 0;
        Collide_GetJointAbsPosition(item, &pos, 0);
        AddDynamicLight(pos.x, pos.y, pos.z, 12, 11);
    }

    Item_Animate(item);
}

void __cdecl Lara_Control_Cutscene(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    item->rot.y = g_Camera.target_angle;
    item->pos.x = g_Camera.pos.pos.x;
    item->pos.y = g_Camera.pos.pos.y;
    item->pos.z = g_Camera.pos.pos.z;

    XYZ_32 pos = { 0 };
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_FindByPos(pos.x, pos.y, pos.z);
    if (room_num != NO_ROOM_NEG && item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    Lara_Animate(item);
}

void __cdecl CutscenePlayer1_Initialise(const int16_t item_num)
{
    OBJECT *const obj = &g_Objects[O_LARA];
    obj->draw_routine = Lara_Draw;
    obj->control = Lara_Control_Cutscene;

    Item_AddActive(item_num);
    ITEM *const item = &g_Items[item_num];
    g_Camera.pos.pos.x = item->pos.x;
    g_Camera.pos.pos.y = item->pos.y;
    g_Camera.pos.pos.z = item->pos.z;
    g_Camera.target_angle = 0;
    g_Camera.pos.room_num = item->room_num;
    g_OriginalRoom = g_Camera.pos.room_num;

    item->rot.y = 0;
    item->dynamic_light = 0;
    item->goal_anim_state = 0;
    item->current_anim_state = 0;
    item->frame_num = 0;
    item->anim_num = 0;

    g_Lara.hit_direction = -1;
}

void __cdecl CutscenePlayerGen_Initialise(const int16_t item_num)
{
    Item_AddActive(item_num);
    ITEM *const item = &g_Items[item_num];
    item->rot.y = 0;
    item->dynamic_light = 0;
}

int32_t __cdecl Level_Initialise(
    const int32_t level_num, const GAMEFLOW_LEVEL_TYPE level_type)
{
    g_GameInfo.current_level.num = level_num;
    g_GameInfo.current_level.type = level_type;

    if (level_type != GFL_TITLE && level_type != GFL_CUTSCENE) {
        g_CurrentLevel = level_num;
    }
    g_IsDemoLevelType = level_type == GFL_DEMO;
    InitialiseGameFlags();
    g_Lara.item_num = NO_ITEM;
    g_IsTitleLoaded = false;

    bool result;
    if (level_type == GFL_TITLE) {
        result = S_LoadLevelFile(g_GF_TitleFileNames[0], level_num, level_type);
    } else if (level_type == GFL_CUTSCENE) {
        result = S_LoadLevelFile(
            g_GF_CutsceneFileNames[level_num], level_num, level_type);
    } else {
        result = S_LoadLevelFile(
            g_GF_LevelFileNames[level_num], level_num, level_type);
    }
    if (!result) {
        return result;
    }

    if (g_Lara.item_num != NO_ITEM) {
        Lara_Initialise(level_type);
    }
    if (level_type == GFL_NORMAL || level_type == GFL_SAVED
        || level_type == GFL_DEMO) {
        GetCarriedItems();
    }
    g_Effects = game_malloc(MAX_EFFECTS * sizeof(FX), GBUF_EFFECTS_ARRAY);
    Effect_InitialiseArray();
    LOT_InitialiseArray();
    Inv_InitColors();
    Overlay_HideGameInfo();
    Overlay_InitialisePickUpDisplay();
    S_InitialiseScreen(level_type);
    g_HealthBarTimer = 100;
    Sound_StopAllSamples();
    if (level_type == GFL_SAVED) {
        ExtractSaveGameInfo();
    } else if (level_type == GFL_NORMAL) {
        GF_ModifyInventory(g_CurrentLevel, 0);
    }

    if (g_Objects[O_FINAL_LEVEL_COUNTER].loaded) {
        InitialiseFinalLevel();
    }

    if (level_type == GFL_NORMAL || level_type == GFL_SAVED
        || level_type == GFL_DEMO) {
        if (g_GF_MusicTracks[0]) {
            Music_Play(g_GF_MusicTracks[0], 1);
        }
    }
    g_IsAssaultTimerActive = 0;
    g_IsAssaultTimerDisplay = 0;
    g_Camera.underwater = 0;
    return true;
}

void __cdecl RestoreLostBuffers(void)
{
    if (g_PrimaryBufferSurface == NULL) {
        Shell_ExitSystem("Oops... no front buffer");
        return;
    }

    bool rebuild = false;

    if (FAILED(DDrawSurfaceRestoreLost(
            g_PrimaryBufferSurface, NULL, g_SavedAppSettings.fullscreen))) {
        rebuild = true;
    }

    if ((g_SavedAppSettings.fullscreen
         || g_SavedAppSettings.render_mode == RM_HARDWARE)
        && FAILED(DDrawSurfaceRestoreLost(
            g_BackBufferSurface, g_PrimaryBufferSurface, true))) {
        rebuild = true;
    }

    if (g_SavedAppSettings.triple_buffering
        && FAILED(DDrawSurfaceRestoreLost(
            g_ThirdBufferSurface, g_PrimaryBufferSurface, true))) {
        rebuild = true;
    }

    if (g_SavedAppSettings.render_mode == RM_SOFTWARE
        && FAILED(
            DDrawSurfaceRestoreLost(g_RenderBufferSurface, NULL, false))) {
        rebuild = true;
    }

    if (g_ZBufferSurface != NULL
        && FAILED(DDrawSurfaceRestoreLost(g_ZBufferSurface, NULL, false))) {
        rebuild = true;
    }

    if (g_PictureBufferSurface != NULL
        && FAILED(
            DDrawSurfaceRestoreLost(g_PictureBufferSurface, NULL, false))) {
        rebuild = true;
    }

    if (rebuild && !g_IsGameToExit) {
        ApplySettings(&g_SavedAppSettings);
        if (g_SavedAppSettings.render_mode == RM_HARDWARE)
            HWR_GetPageHandles();
    }
}

void __cdecl CreateScreenBuffers(void)
{
    {
        DDSDESC dsp = {
            .dwSize = sizeof(DDSDESC),
            .dwFlags = DDSD_BACKBUFFERCOUNT | DDSD_CAPS,
            .dwBackBufferCount = (g_SavedAppSettings.triple_buffering) ? 2 : 1,
            .ddsCaps.dwCaps =
                DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX,
        };
        if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
            dsp.ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;
        }
        if (FAILED(DDrawSurfaceCreate(&dsp, &g_PrimaryBufferSurface))) {
            Shell_ExitSystem("Failed to create primary screen buffer");
        }
        WinVidClearBuffer(g_PrimaryBufferSurface, NULL, 0);
    }

    {
        DDSCAPS caps = {
            .dwCaps = DDSCAPS_BACKBUFFER,
        };
        if (FAILED(IDirectDrawSurface_GetAttachedSurface(
                g_PrimaryBufferSurface, &caps, &g_BackBufferSurface))) {
            Shell_ExitSystem("Failed to create back screen buffer");
        }
        WinVidClearBuffer(g_BackBufferSurface, NULL, 0);
    }

    if (g_SavedAppSettings.triple_buffering) {
        DDSCAPS caps = {
            .dwCaps = DDSCAPS_FLIP,
        };
        if (FAILED(IDirectDrawSurface_GetAttachedSurface(
                g_BackBufferSurface, &caps, &g_ThirdBufferSurface))) {
            Shell_ExitSystem("Failed to create third screen buffer");
        }
        WinVidClearBuffer(g_ThirdBufferSurface, NULL, 0);
    }
}

void __cdecl CreatePrimarySurface(void)
{
    if ((g_GameVid_IsVga && g_SavedAppSettings.render_mode == RM_HARDWARE)
        || (!g_GameVid_IsVga
            && g_SavedAppSettings.render_mode == RM_SOFTWARE)) {
        Shell_ExitSystem("Wrong bit depth");
    }

    DDSDESC dsp = {
        .dwSize = sizeof(DDSDESC),
        .dwFlags = DDSD_CAPS,
        .ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE,
    };

    if (FAILED(DDrawSurfaceCreate(&dsp, &g_PrimaryBufferSurface))) {
        Shell_ExitSystem("Failed to create primary screen buffer");
    }
}

void __cdecl CreateBackBuffer(void)
{
    DDSDESC dsp = {
        .dwSize = sizeof(DDSDESC),
        .dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS,
        .dwWidth = g_GameVid_BufWidth,
        .dwHeight = g_GameVid_BufHeight,
        .ddsCaps = {
            .dwCaps = DDSCAPS_3DDEVICE|DDSCAPS_OFFSCREENPLAIN,
        },
    };
    if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
        dsp.ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
    }
    if (FAILED(DDrawSurfaceCreate(&dsp, &g_BackBufferSurface))) {
        Shell_ExitSystem("Failed to create back screen buffer");
    }
    WinVidClearBuffer(g_BackBufferSurface, 0, 0);
}

void __cdecl CreateClipper(void)
{
    if (FAILED(IDirectDraw_CreateClipper(g_DDraw, 0, &g_DDrawClipper, NULL))) {
        Shell_ExitSystem("Failed to create clipper");
    }

    if (FAILED(IDirectDrawClipper_SetHWnd(
            g_DDrawClipper, 0, g_GameWindowHandle))) {
        Shell_ExitSystem("Failed to attach clipper to the game window");
    }

    if (FAILED(IDirectDrawSurface_SetClipper(
            g_PrimaryBufferSurface, g_DDrawClipper))) {
        Shell_ExitSystem("Failed to attach clipper to the primary surface");
    }
}

void __cdecl CreateWindowPalette(void)
{
    memset(g_WinVid_Palette, 0, sizeof(g_WinVid_Palette));

    DWORD flags = DDPCAPS_8BIT;
    if (g_GameVid_IsWindowedVGA) {
        for (int32_t i = 0; i < 10; i++) {
            g_WinVid_Palette[i].peFlags = PC_EXPLICIT;
            g_WinVid_Palette[i].peRed = i;
        }
        for (int32_t i = 10; i < 246; i++) {
            g_WinVid_Palette[i].peFlags = PC_NOCOLLAPSE | PC_RESERVED;
        }
        for (int32_t i = 246; i < 256; i++) {
            g_WinVid_Palette[i].peFlags = PC_EXPLICIT;
            g_WinVid_Palette[i].peRed = i; // TODO: i - 246?
        }
    } else {
        for (int32_t i = 0; i < 256; i++) {
            g_WinVid_Palette[i].peFlags = PC_RESERVED;
        }
        flags |= DDPCAPS_ALLOW256;
    }

    if (FAILED(IDirectDraw_CreatePalette(
            g_DDraw, flags, g_WinVid_Palette, &g_DDrawPalette, 0))) {
        Shell_ExitSystem("Failed to create palette");
    }

    if (FAILED(IDirectDrawSurface_SetPalette(
            g_PrimaryBufferSurface, g_DDrawPalette))) {
        Shell_ExitSystem("Failed to attach palette to the primary buffer");
    }
}

void __cdecl CreateZBuffer(void)
{
    if ((g_CurrentDisplayAdapter.hw_device_desc.dpcTriCaps.dwRasterCaps
         & D3DPRASTERCAPS_ZBUFFERLESSHSR)
        != 0) {
        return;
    }

    DDSDESC dsp = {
        .dwSize = sizeof(DDSDESC),
        .dwWidth = g_GameVid_BufWidth,
        .dwHeight = g_GameVid_BufHeight,
        .dwFlags = DDSD_ZBUFFERBITDEPTH | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS,
        .dwZBufferBitDepth = GetZBufferDepth(),
        .ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY,
    };

    if (FAILED(DDrawSurfaceCreate(&dsp, &g_ZBufferSurface))) {
        Shell_ExitSystem("Failed to create z-buffer");
    }

    if (FAILED(IDirectDrawSurface_AddAttachedSurface(
            g_BackBufferSurface, g_ZBufferSurface))) {
        Shell_ExitSystem("Failed to attach z-buffer to the back buffer");
    }
}

int32_t __cdecl GetZBufferDepth(void)
{
    const int32_t bit_depth_mask =
        g_CurrentDisplayAdapter.hw_device_desc.dwDeviceZBufferBitDepth;
    if (bit_depth_mask & DDBD_16) {
        return 16;
    }
    if (bit_depth_mask & DDBD_24) {
        return 24;
    }
    if (bit_depth_mask & DDBD_32) {
        return 32;
    }
    return 8;
}

void __cdecl CreateRenderBuffer(void)
{
    DDSDESC dsp = {
        .dwSize = sizeof(DDSDESC),
        .dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS,
        .dwWidth = g_GameVid_BufWidth,
        .dwHeight = g_GameVid_BufHeight,
        .ddsCaps = {
            .dwCaps = DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN,
        },
    };

    if (FAILED(DDrawSurfaceCreate(&dsp, &g_RenderBufferSurface))) {
        Shell_ExitSystem("Failed to create render buffer");
    }

    if (!WinVidClearBuffer(g_RenderBufferSurface, NULL, 0)) {
        Shell_ExitSystem("Failed to clear render buffer");
    }
}

void __cdecl CreatePictureBuffer(void)
{
    DDSDESC dsp = {
        .dwSize = sizeof(DDSDESC),
        .dwFlags = DDSD_WIDTH|DDSD_HEIGHT|DDSD_CAPS,
        .dwWidth = 640,
        .dwHeight = 480,
        .ddsCaps = {
            .dwCaps = DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN,
        },
    };

    if (FAILED(DDrawSurfaceCreate(&dsp, &g_PictureBufferSurface))) {
        Shell_ExitSystem("Failed to create picture buffer");
    }
}

void __cdecl ClearBuffers(DWORD flags, DWORD fill_color)
{
    RECT win_rect;
    if (flags & CLRB_PHDWINSIZE) {
        win_rect.left = g_PhdWinMinX;
        win_rect.top = g_PhdWinMinY;
        win_rect.right = g_PhdWinMinX + g_PhdWinWidth;
        win_rect.bottom = g_PhdWinMinY + g_PhdWinHeight;
    } else {
        win_rect.left = 0;
        win_rect.top = 0;
        win_rect.right = g_GameVid_Width;
        win_rect.bottom = g_GameVid_Height;
    }

    if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
        DWORD d3d_clear_flags = 0;
        if (flags & CLRB_BACK_BUFFER) {
            d3d_clear_flags |= D3DCLEAR_TARGET;
        }

        if (flags & CLRB_Z_BUFFER) {
            d3d_clear_flags |= D3DCLEAR_ZBUFFER;
        }

        if (d3d_clear_flags != 0) {
            D3DRECT d3d_rect = {
                .x1 = win_rect.left,
                .y1 = win_rect.top,
                .x2 = win_rect.right,
                .y2 = win_rect.bottom,
            };
            IDirect3DViewport_Clear(g_D3DView, 1, &d3d_rect, d3d_clear_flags);
        }
    } else {
        if (flags & CLRB_BACK_BUFFER) {
            WinVidClearBuffer(g_BackBufferSurface, &win_rect, fill_color);
        }
    }

    if (flags & CLRB_PRIMARY_BUFFER) {
        WinVidClearBuffer(g_PrimaryBufferSurface, &win_rect, fill_color);
    }

    if (flags & CLRB_THIRD_BUFFER) {
        WinVidClearBuffer(g_ThirdBufferSurface, &win_rect, fill_color);
    }

    if (flags & CLRB_RENDER_BUFFER) {
        WinVidClearBuffer(g_RenderBufferSurface, &win_rect, fill_color);
    }

    if (flags & CLRB_PICTURE_BUFFER) {
        win_rect.left = 0;
        win_rect.top = 0;
        win_rect.right = 640;
        win_rect.bottom = 480;
        WinVidClearBuffer(g_PictureBufferSurface, &win_rect, fill_color);
    }

    if (flags & CLRB_WINDOWED_PRIMARY_BUFFER) {
        win_rect.left = g_GameWindowPositionX;
        win_rect.top = g_GameWindowPositionY;
        win_rect.right = g_GameWindowPositionX + g_GameWindowWidth;
        win_rect.bottom = g_GameWindowPositionY + g_GameWindowHeight;
        WinVidClearBuffer(g_PrimaryBufferSurface, &win_rect, fill_color);
    }
}

void __cdecl UpdateFrame(const bool need_run_message_loop, LPRECT rect)
{
    if (rect == NULL) {
        rect = &g_GameVid_Rect;
    }

    RestoreLostBuffers();
    if (g_SavedAppSettings.fullscreen) {
        if (g_SavedAppSettings.render_mode == RM_SOFTWARE) {
            IDirectDrawSurface_Blt(
                g_BackBufferSurface, rect, g_RenderBufferSurface, rect,
                DDBLT_WAIT, NULL);
        }
        IDirectDrawSurface_Flip(g_PrimaryBufferSurface, NULL, DDFLIP_WAIT);
    } else {
        RECT dst_rect;
        dst_rect.left = g_GameWindowPositionX + rect->left;
        dst_rect.top = g_GameWindowPositionY + rect->top;
        dst_rect.bottom = g_GameWindowPositionY + rect->bottom;
        dst_rect.right = g_GameWindowPositionX + rect->right;
        LPDDS dst_surface = g_SavedAppSettings.render_mode == RM_SOFTWARE
            ? g_RenderBufferSurface
            : g_BackBufferSurface;
        IDirectDrawSurface_Blt(
            g_PrimaryBufferSurface, &dst_rect, dst_surface, rect, DDBLT_WAIT,
            NULL);
    }

    if (need_run_message_loop) {
        WinVidSpinMessageLoop(false);
    }
}

void __cdecl WaitPrimaryBufferFlip(void)
{
    if (g_SavedAppSettings.flip_broken && g_SavedAppSettings.fullscreen) {
        while (IDirectDrawSurface_GetFlipStatus(
                   g_PrimaryBufferSurface, DDGFS_ISFLIPDONE)
               == DDERR_WASSTILLDRAWING) { }
    }
}

bool __cdecl RenderInit(void)
{
    return true;
}

void __cdecl RenderStart(const bool is_reset)
{
    if (is_reset) {
        g_NeedToReloadTextures = false;
    }

    if (g_SavedAppSettings.fullscreen) {
        assert(g_SavedAppSettings.video_mode != NULL);

        DISPLAY_MODE disp_mode = g_SavedAppSettings.video_mode->body;

        const bool result = WinVidGoFullScreen(&disp_mode);
        assert(result);

        CreateScreenBuffers();
        g_GameVid_Width = disp_mode.width;
        g_GameVid_Height = disp_mode.height;
        g_GameVid_BPP = disp_mode.bpp;
        g_GameVid_BufWidth = disp_mode.width;
        g_GameVid_BufHeight = disp_mode.height;
        g_GameVid_IsVga = disp_mode.vga != VGA_NO_VGA;
        g_GameVid_IsWindowedVGA = false;
        g_GameVid_IsFullscreenVGA = disp_mode.vga == VGA_STANDARD;
    } else {
        int32_t min_width = 320;
        int32_t min_height = CalculateWindowHeight(320, 200);
        if (min_height < 200) {
            min_width = CalculateWindowWidth(320, 200);
            min_height = 200;
        }

        WinVidSetMinWindowSize(min_width, min_height);

        DISPLAY_MODE disp_mode;
        const bool result = WinVidGoWindowed(
            g_SavedAppSettings.window_width, g_SavedAppSettings.window_height,
            &disp_mode);
        assert(result);

        g_GameVid_Width = disp_mode.width;
        g_GameVid_Height = disp_mode.height;
        g_GameVid_BPP = disp_mode.bpp;

        g_GameVid_BufWidth = (disp_mode.width + 0x1F) & ~0x1F;
        g_GameVid_BufHeight = (disp_mode.height + 0x1F) & ~0x1F;
        g_GameVid_IsVga = disp_mode.vga != 0;
        g_GameVid_IsWindowedVGA = disp_mode.vga != VGA_NO_VGA;
        g_GameVid_IsFullscreenVGA = false;

        CreatePrimarySurface();
        if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
            CreateBackBuffer();
        }
        CreateClipper();
    }

    DDPIXELFORMAT pixel_format = { 0 };
    pixel_format.dwSize = sizeof(DDPIXELFORMAT);
    const HRESULT result = IDirectDrawSurface_GetPixelFormat(
        g_PrimaryBufferSurface, &pixel_format);
    if (FAILED(result)) {
        Shell_ExitSystem("GetPixelFormat() failed");
    }

    WinVidGetColorBitMasks(&g_ColorBitMasks, &pixel_format);
    if (g_GameVid_IsVga) {
        CreateWindowPalette();
    }

    if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
        if (g_SavedAppSettings.zbuffer) {
            CreateZBuffer();
        }
        D3DDeviceCreate(g_BackBufferSurface);
        EnumerateTextureFormats();
    } else {
        CreateRenderBuffer();
        if (g_PictureBufferSurface == NULL) {
            CreatePictureBuffer();
        }
    }

    if (g_NeedToReloadTextures) {
        bool is_16bit_textures = g_TextureFormat.bpp >= 16;
        if (g_IsWindowedVGA != g_GameVid_IsWindowedVGA
            || g_Is16bitTextures != is_16bit_textures) {
            S_ReloadLevelGraphics(
                g_IsWindowedVGA != g_GameVid_IsWindowedVGA,
                g_Is16bitTextures != is_16bit_textures);
            g_IsWindowedVGA = g_GameVid_IsWindowedVGA;
            g_Is16bitTextures = is_16bit_textures;
        } else if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
            ReloadTextures(true);
            HWR_GetPageHandles();
        }
    } else {
        g_IsWindowedVGA = g_GameVid_IsWindowedVGA;
        g_Is16bitTextures = g_TextureFormat.bpp >= 16;
    }

    g_GameVid_BufRect.left = 0;
    g_GameVid_BufRect.top = 0;
    g_GameVid_BufRect.right = g_GameVid_BufWidth;
    g_GameVid_BufRect.bottom = g_GameVid_BufHeight;

    g_GameVid_Rect.left = 0;
    g_GameVid_Rect.top = 0;
    g_GameVid_Rect.right = g_GameVid_Width;
    g_GameVid_Rect.bottom = g_GameVid_Height;

    g_DumpWidth = g_GameVid_Width;
    g_DumpHeight = g_GameVid_Height;

    setup_screen_size();
    g_NeedToReloadTextures = true;
}

void __cdecl RenderFinish(bool need_to_clear_textures)
{
    if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
        if (need_to_clear_textures) {
            HWR_FreeTexturePages();
            CleanupTextures();
        }

        Direct3DRelease();
        if (g_ZBufferSurface != NULL) {
            IDirectDrawSurface_Release(g_ZBufferSurface);
            g_ZBufferSurface = NULL;
        }
    } else {
        if (need_to_clear_textures && g_PictureBufferSurface != NULL) {
            IDirectDrawSurface_Release(g_PictureBufferSurface);
            g_PictureBufferSurface = NULL;
        }

        if (g_RenderBufferSurface != NULL) {
            IDirectDrawSurface_Release(g_RenderBufferSurface);
            g_RenderBufferSurface = NULL;
        }
    }

    if (g_DDrawPalette != NULL) {
        IDirectDrawPalette_Release(g_DDrawPalette);
        g_DDrawPalette = NULL;
    }

    if (g_DDrawClipper != NULL) {
        IDirectDrawClipper_Release(g_DDrawClipper);
        g_DDrawClipper = NULL;
    }

    if (g_ThirdBufferSurface != NULL) {
        IDirectDrawSurface_Release(g_ThirdBufferSurface);
        g_ThirdBufferSurface = NULL;
    }

    if (g_BackBufferSurface != NULL) {
        IDirectDrawSurface_Release(g_BackBufferSurface);
        g_BackBufferSurface = NULL;
    }

    if (g_PrimaryBufferSurface != NULL) {
        IDirectDrawSurface_Release(g_PrimaryBufferSurface);
        g_PrimaryBufferSurface = NULL;
    }

    if (need_to_clear_textures) {
        g_NeedToReloadTextures = false;
    }
}

bool __cdecl ApplySettings(const APP_SETTINGS *const new_settings)
{
    char mode_string[64] = { 0 };
    APP_SETTINGS old_settings = g_SavedAppSettings;

    RenderFinish(false);

    if (new_settings != &g_SavedAppSettings)
        g_SavedAppSettings = *new_settings;

    RenderStart(false);
    S_InitialiseScreen(GFL_NO_LEVEL);

    if (g_SavedAppSettings.render_mode != old_settings.render_mode) {
        S_ReloadLevelGraphics(1, 1);
    } else if (
        g_SavedAppSettings.render_mode == RM_SOFTWARE
        && g_SavedAppSettings.fullscreen != old_settings.fullscreen) {
        S_ReloadLevelGraphics(1, 0);
    }

    if (g_SavedAppSettings.fullscreen) {
        sprintf(
            mode_string, "%dx%dx%d", g_GameVid_Width, g_GameVid_Height,
            g_GameVid_BPP);
    } else {
        sprintf(mode_string, "%dx%d", g_GameVid_Width, g_GameVid_Height);
    }

    Overlay_DisplayModeInfo(mode_string);
    return true;
}

void __cdecl FmvBackToGame(void)
{
    RenderStart(true);
}

void __cdecl GameApplySettings(APP_SETTINGS *const new_settings)
{
    bool need_init_render_state = false;
    bool need_adjust_texel = false;
    bool need_rebuild_buffers = false;

    if (new_settings->preferred_display_adapter
            != g_SavedAppSettings.preferred_display_adapter
        || new_settings->preferred_sound_adapter
            != g_SavedAppSettings.preferred_sound_adapter
        || new_settings->preferred_joystick
            != g_SavedAppSettings.preferred_joystick) {
        return;
    }

    if (new_settings->render_mode != g_SavedAppSettings.render_mode
        || new_settings->video_mode != g_SavedAppSettings.video_mode
        || new_settings->fullscreen != g_SavedAppSettings.fullscreen
        || new_settings->zbuffer != g_SavedAppSettings.zbuffer
        || new_settings->triple_buffering
            != g_SavedAppSettings.triple_buffering) {
        ApplySettings(new_settings);
        S_AdjustTexelCoordinates();
        return;
    }

    if (new_settings->perspective_correct
            != g_SavedAppSettings.perspective_correct
        || new_settings->dither != g_SavedAppSettings.dither
        || new_settings->bilinear_filtering
            != g_SavedAppSettings.bilinear_filtering) {
        need_init_render_state = true;
    }

    if (new_settings->bilinear_filtering
            != g_SavedAppSettings.bilinear_filtering
        || new_settings->render_mode != g_SavedAppSettings.render_mode) {
        need_adjust_texel = true;
    }

    if (!new_settings->fullscreen) {
        if (new_settings->window_width != g_SavedAppSettings.window_width
            || new_settings->window_height
                != g_SavedAppSettings.window_height) {
            DISPLAY_MODE disp_mode;
            if (!WinVidGoWindowed(
                    new_settings->window_width, new_settings->window_height,
                    &disp_mode)) {
                return;
            }
            new_settings->window_width = disp_mode.width;
            new_settings->window_height = disp_mode.height;
            if (new_settings->window_width != g_SavedAppSettings.window_width
                || new_settings->window_height
                    != g_SavedAppSettings.window_height) {
                if (g_GameVid_BufWidth - new_settings->window_width < 0
                    || g_GameVid_BufWidth - new_settings->window_width > 64
                    || g_GameVid_BufHeight - new_settings->window_height < 0
                    || g_GameVid_BufHeight - new_settings->window_height > 64) {
                    need_rebuild_buffers = true;
                } else {
                    g_SavedAppSettings.window_width =
                        new_settings->window_width;
                    g_SavedAppSettings.window_height =
                        new_settings->window_height;
                    g_GameVid_Width = new_settings->window_width;
                    g_GameVid_Height = new_settings->window_height;
                    g_GameVid_Rect.right = new_settings->window_width;
                    g_GameVid_Rect.bottom = new_settings->window_height;
                    if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
                        D3DSetViewport();
                    }
                    setup_screen_size();
                    g_WinVidNeedToResetBuffers = false;
                }
            }
        }
    }

    if (need_init_render_state) {
        g_SavedAppSettings.perspective_correct =
            new_settings->perspective_correct;
        g_SavedAppSettings.dither = new_settings->dither;
        g_SavedAppSettings.bilinear_filtering =
            new_settings->bilinear_filtering;
        if (g_SavedAppSettings.render_mode == RM_HARDWARE) {
            HWR_InitState();
        }
    }

    if (need_rebuild_buffers) {
        ClearBuffers(CLRB_WINDOWED_PRIMARY_BUFFER, 0);
        ApplySettings(new_settings);
    }

    if (need_adjust_texel) {
        S_AdjustTexelCoordinates();
    }
}

void __cdecl UpdateGameResolution(void)
{
    APP_SETTINGS new_settings = g_SavedAppSettings;
    new_settings.window_width = g_GameWindowWidth;
    new_settings.window_height = g_GameWindowHeight;
    GameApplySettings(&new_settings);
    char mode_string[64] = { 0 };
    sprintf(mode_string, "%dx%d", g_GameVid_Width, g_GameVid_Height);
    Overlay_DisplayModeInfo(mode_string);
}

bool __cdecl D3DCreate(void)
{
    const HRESULT rc =
        IDirectDraw_QueryInterface(g_DDraw, &IID_IDirect3D2, (LPVOID *)&g_D3D);
    return SUCCEEDED(rc);
}

void __cdecl D3DRelease(void)
{
    if (g_D3D != NULL) {
        IDirect3D_Release(g_D3D);
        g_D3D = NULL;
    }
}

void __cdecl Enumerate3DDevices(DISPLAY_ADAPTER *const adapter)
{
    if (D3DCreate()) {
        g_D3D->lpVtbl->EnumDevices(
            g_D3D, (void *)Enum3DDevicesCallback, (LPVOID)adapter);
        D3DRelease();
    }
}

HRESULT __stdcall Enum3DDevicesCallback(
    GUID FAR *lpGuid, LPTSTR lpDeviceDescription, LPTSTR lpDeviceName,
    LPD3DDEVICEDESC_V2 lpD3DHWDeviceDesc, LPD3DDEVICEDESC_V2 lpD3DHELDeviceDesc,
    LPVOID lpContext)
{
    DISPLAY_ADAPTER *adapter = (DISPLAY_ADAPTER *)lpContext;

    if (lpD3DHWDeviceDesc != NULL && D3DIsSupported(lpD3DHWDeviceDesc)) {
        adapter->hw_render_supported = true;
        adapter->device_guid = *lpGuid;
        adapter->hw_device_desc = *lpD3DHWDeviceDesc;

        adapter->perspective_correct_supported =
            (lpD3DHWDeviceDesc->dpcTriCaps.dwTextureCaps
             & D3DPTEXTURECAPS_PERSPECTIVE)
            ? true
            : false;
        adapter->dither_supported =
            (lpD3DHWDeviceDesc->dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_DITHER)
            ? true
            : false;
        adapter->zbuffer_supported =
            (lpD3DHWDeviceDesc->dwDeviceZBufferBitDepth) ? true : false;
        adapter->linear_filter_supported =
            (lpD3DHWDeviceDesc->dpcTriCaps.dwTextureFilterCaps
             & D3DPTFILTERCAPS_LINEAR)
            ? true
            : false;
        adapter->shade_restricted =
            (lpD3DHWDeviceDesc->dpcTriCaps.dwShadeCaps
             & (D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_ALPHAFLATBLEND))
            ? false
            : true;
    }

    return D3DENUMRET_OK;
}

bool __cdecl D3DIsSupported(LPD3DDEVICEDESC_V2 desc)
{
    return (desc->dwFlags & D3DDD_COLORMODEL)
        && (desc->dcmColorModel & D3DCOLOR_RGB)
        && (desc->dwFlags & D3DDD_TRICAPS)
        && (desc->dpcTriCaps.dwShadeCaps & D3DPSHADECAPS_COLORGOURAUDRGB)
        && (desc->dpcTriCaps.dwTextureBlendCaps & D3DPTBLENDCAPS_MODULATE);
}

bool __cdecl D3DSetViewport(void)
{
    D3DVIEWPORT2 viewPort = {

        .dwSize = sizeof(D3DVIEWPORT2),
        .dvClipX = 0.0,
        .dvClipY = 0.0,
        .dvClipWidth = (float)g_GameVid_Width,
        .dvClipHeight = (float)g_GameVid_Height,

        .dwX = 0,
        .dwY = 0,
        .dwWidth = g_GameVid_Width,
        .dwHeight = g_GameVid_Height,

        .dvMinZ = 0.0,
        .dvMaxZ = 1.0,
    };

    HRESULT rc = IDirect3DViewport2_SetViewport2(g_D3DView, &viewPort);
    if (FAILED(rc)) {
        IDirect3DViewport2_GetViewport2(g_D3DView, &viewPort);
        return false;
    }

    rc = IDirect3DDevice2_SetCurrentViewport(g_D3DDev, g_D3DView);
    return SUCCEEDED(rc);
}

void __cdecl D3DDeviceCreate(LPDDS lpBackBuffer)
{
    if (g_D3D == NULL && !D3DCreate()) {
        Shell_ExitSystem("Failed to create D3D");
    }

    if (FAILED(g_D3D->lpVtbl->CreateDevice(
            g_D3D, &IID_IDirect3DHALDevice, (LPDIRECTDRAWSURFACE)lpBackBuffer,
            &g_D3DDev))) {
        Shell_ExitSystem("Failed to create device");
    }

    if (FAILED(g_D3D->lpVtbl->CreateViewport(g_D3D, &g_D3DView, NULL))) {
        Shell_ExitSystem("Failed to create viewport");
    }

    if (FAILED(g_D3DDev->lpVtbl->AddViewport(g_D3DDev, g_D3DView))) {
        Shell_ExitSystem("Failed to add viewport");
    }

    if (FAILED(!D3DSetViewport())) {
        Shell_ExitSystem("Failed to set viewport");
    }

    if (FAILED(g_D3D->lpVtbl->CreateMaterial(g_D3D, &g_D3DMaterial, NULL))) {
        Shell_ExitSystem("Failed to create material");
    }

    D3DMATERIALHANDLE mat_handle;
    D3DMATERIAL mat_data = { 0 };
    mat_data.dwSize = sizeof(mat_data);

    if (FAILED(g_D3DMaterial->lpVtbl->SetMaterial(g_D3DMaterial, &mat_data))) {
        Shell_ExitSystem("Failed to set material");
    }

    if (FAILED(g_D3DMaterial->lpVtbl->GetHandle(
            g_D3DMaterial, g_D3DDev, &mat_handle))) {
        Shell_ExitSystem("Failed to get material handle");
    }

    if (FAILED(g_D3DView->lpVtbl->SetBackground(g_D3DView, mat_handle))) {
        Shell_ExitSystem("Failed to set material background");
    }
}

void __cdecl Direct3DRelease(void)
{
    if (g_D3DMaterial != NULL) {
        g_D3DMaterial->lpVtbl->Release(g_D3DMaterial);
        g_D3DMaterial = NULL;
    }

    if (g_D3DView != NULL) {
        g_D3DView->lpVtbl->Release(g_D3DView);
        g_D3DView = NULL;
    }

    if (g_D3DDev != NULL) {
        g_D3DDev->lpVtbl->Release(g_D3DDev);
        g_D3DDev = NULL;
    }

    D3DRelease();
}

bool __cdecl Direct3DInit(void)
{
    return true;
}

bool __cdecl DDrawCreate(LPGUID lpGUID)
{
    if (FAILED(DirectDrawCreate(lpGUID, &g_DDrawInterface, 0))) {
        return false;
    }

    if (FAILED(g_DDrawInterface->lpVtbl->QueryInterface(
            g_DDrawInterface, &IID_IDirectDraw2, (LPVOID *)&g_DDraw))) {
        return false;
    }

    g_DDraw->lpVtbl->SetCooperativeLevel(
        g_DDraw, g_GameWindowHandle, DDSCL_NORMAL);
    return true;
}

void __cdecl DDrawRelease(void)
{
    if (g_DDraw != NULL) {
        g_DDraw->lpVtbl->Release(g_DDraw);
        g_DDraw = NULL;
    }
    if (g_DDrawInterface != NULL) {
        g_DDrawInterface->lpVtbl->Release(g_DDrawInterface);
        g_DDrawInterface = NULL;
    }
}

void __cdecl GameWindowCalculateSizeFromClient(
    int32_t *const width, int32_t *const height)
{
    RECT rect = { 0, 0, *width, *height };
    const DWORD style = GetWindowLong(g_GameWindowHandle, GWL_STYLE);
    const DWORD style_ex = GetWindowLong(g_GameWindowHandle, GWL_EXSTYLE);
    AdjustWindowRectEx(&rect, style, FALSE, style_ex);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
}

void __cdecl GameWindowCalculateSizeFromClientByZero(
    int32_t *const width, int32_t *const height)
{
    RECT rect = { 0, 0, 0, 0 };
    const DWORD style = GetWindowLong(g_GameWindowHandle, GWL_STYLE);
    const DWORD styleEx = GetWindowLong(g_GameWindowHandle, GWL_EXSTYLE);
    AdjustWindowRectEx(&rect, style, FALSE, styleEx);
    *width += rect.left - rect.right;
    *height += rect.top - rect.bottom;
}

bool __thiscall CompareVideoModes(
    const DISPLAY_MODE *const mode1, const DISPLAY_MODE *const mode2)
{
    const int32_t square1 = mode1->width * mode1->height;
    const int32_t square2 = mode2->width * mode2->height;
    if (square1 < square2) {
        return true;
    }
    if (square1 > square2) {
        return false;
    }
    if (mode1->bpp < mode2->bpp) {
        return true;
    }
    if (mode1->bpp > mode2->bpp) {
        return false;
    }
    if (mode1->vga < mode2->vga) {
        return true;
    }
    if (mode1->vga > mode2->vga) {
        return false;
    }
    return false;
}

bool __cdecl WinVidGetDisplayModes(void)
{
    for (DISPLAY_ADAPTER_NODE *adapter = g_DisplayAdapterList.head; adapter;
         adapter = adapter->next) {
        DDrawCreate(adapter->body.adapter_guid_ptr);
        ShowDDrawGameWindow(false);
        g_DDraw->lpVtbl->EnumDisplayModes(
            g_DDraw, DDEDM_STANDARDVGAMODES, NULL, (LPVOID)&adapter->body,
            EnumDisplayModesCallback);
        HideDDrawGameWindow();
        DDrawRelease();
    }
    return true;
}

HRESULT __stdcall EnumDisplayModesCallback(
    LPDDSDESC lpDDSurfaceDesc, LPVOID lpContext)
{
    DISPLAY_ADAPTER *adapter = (DISPLAY_ADAPTER *)lpContext;
    VGA_MODE vga_mode = VGA_NO_VGA;
    bool sw_renderer_supported = false;

    if (!(lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT)
        || !(lpDDSurfaceDesc->dwFlags & DDSD_WIDTH)
        || !(lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
        || !(lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_RGB)) {
        return DDENUMRET_OK;
    }

    if ((lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) != 0
        && lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount == 8) {
        if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_MODEX) {
            vga_mode = VGA_MODEX;
        } else if (lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_STANDARDVGAMODE) {
            vga_mode = VGA_STANDARD;
        } else {
            vga_mode = VGA_256_COLOR;
        }

        if (lpDDSurfaceDesc->dwWidth == 320 && lpDDSurfaceDesc->dwHeight == 200
            && (!adapter->is_vga_mode1_presented
                || vga_mode < adapter->vga_mode1.vga)) {
            adapter->vga_mode1.width = 320;
            adapter->vga_mode1.height = 200;
            adapter->vga_mode1.bpp = 8;
            adapter->vga_mode1.vga = vga_mode;
            adapter->is_vga_mode1_presented = true;
        }

        if (lpDDSurfaceDesc->dwWidth == 640 && lpDDSurfaceDesc->dwHeight == 480
            && (!adapter->is_vga_mode2_presented
                || vga_mode < adapter->vga_mode2.vga)) {
            adapter->vga_mode2.width = 640;
            adapter->vga_mode2.height = 480;
            adapter->vga_mode2.bpp = 8;
            adapter->vga_mode2.vga = vga_mode;
            adapter->is_vga_mode2_presented = true;
        }
        sw_renderer_supported = true;
    }

    DISPLAY_MODE video_mode = {
        .width = lpDDSurfaceDesc->dwWidth,
        .height = lpDDSurfaceDesc->dwHeight,
        .bpp = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount,
        .vga = vga_mode,
    };

    int32_t render_bit_depth =
        GetRenderBitDepth(lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount);

    if (adapter->hw_render_supported
        && (render_bit_depth & adapter->hw_device_desc.dwDeviceRenderBitDepth)
            != 0) {
        M_InsertDisplayModeInListSorted(
            &adapter->hw_disp_mode_list, &video_mode);
    }

    if (sw_renderer_supported) {
        M_InsertDisplayModeInListSorted(
            &adapter->sw_disp_mode_list, &video_mode);
    }

    return DDENUMRET_OK;
}

bool __cdecl WinVidInit(void)
{
    g_AppResultCode = 0;
    // clang-format off
    return WinVidRegisterGameWindowClass()
        && WinVidCreateGameWindow()
        && WinVidGetDisplayAdapters()
        && g_DisplayAdapterList.count
        && WinVidGetDisplayModes();
    // clang-format on
}

bool __cdecl WinVidGetDisplayAdapters(void)
{
    for (DISPLAY_ADAPTER_NODE *node = g_DisplayAdapterList.head,
                              *next_node = NULL;
         node != NULL; node = next_node) {
        next_node = node->next;
        M_DisplayModeListDelete(&node->body.sw_disp_mode_list);
        M_DisplayModeListDelete(&node->body.hw_disp_mode_list);
        S_FlaggedString_Delete(&node->body.driver_name);
        S_FlaggedString_Delete(&node->body.driver_desc);
        free(node);
    }

    g_DisplayAdapterList.head = NULL;
    g_DisplayAdapterList.tail = NULL;
    g_DisplayAdapterList.count = 0;

    g_PrimaryDisplayAdapter = NULL;

    if (!EnumerateDisplayAdapters(&g_DisplayAdapterList)) {
        return false;
    }

    for (DISPLAY_ADAPTER_NODE *node = g_DisplayAdapterList.head; node != NULL;
         node = node->next) {
        if (node->body.adapter_guid_ptr == NULL) {
            g_PrimaryDisplayAdapter = node;
            return true;
        }
    }
    return false;
}

bool __cdecl EnumerateDisplayAdapters(
    DISPLAY_ADAPTER_LIST *display_adapter_list)
{
    return SUCCEEDED(DirectDrawEnumerate(
        EnumDisplayAdaptersCallback, (LPVOID)display_adapter_list));
}

BOOL WINAPI EnumDisplayAdaptersCallback(
    GUID FAR *lpGUID, LPTSTR lpDriverDescription, LPTSTR lpDriverName,
    LPVOID lpContext)
{
    DISPLAY_ADAPTER_NODE *list_node = malloc(sizeof(DISPLAY_ADAPTER_NODE));
    DISPLAY_ADAPTER_LIST *adapter_list = (DISPLAY_ADAPTER_LIST *)lpContext;

    if (list_node == NULL || !DDrawCreate(lpGUID)) {
        return TRUE;
    }

    DDCAPS_DX5 driver_caps = { .dwSize = sizeof(DDCAPS_DX5), 0 };
    DDCAPS_DX5 hel_caps = { .dwSize = sizeof(DDCAPS_DX5), 0 };
    if (FAILED(g_DDraw->lpVtbl->GetCaps(
            g_DDraw, (void *)&driver_caps, (void *)&hel_caps))) {
        goto cleanup;
    }

    list_node->next = NULL;
    list_node->previous = adapter_list->tail;

    S_FlaggedString_InitAdapter(&list_node->body);
    M_DisplayModeListInit(&list_node->body.hw_disp_mode_list);
    M_DisplayModeListInit(&list_node->body.sw_disp_mode_list);

    if (!adapter_list->head) {
        adapter_list->head = list_node;
    }

    if (adapter_list->tail) {
        adapter_list->tail->next = list_node;
    }

    adapter_list->tail = list_node;
    adapter_list->count++;

    if (lpGUID != NULL) {
        list_node->body.adapter_guid = *lpGUID;
        list_node->body.adapter_guid_ptr = &list_node->body.adapter_guid;
    } else {
        memset(&list_node->body.adapter_guid, 0, sizeof(GUID));
        list_node->body.adapter_guid_ptr = NULL;
    }

    lstrcpy(list_node->body.driver_desc.content, lpDriverDescription);
    lstrcpy(list_node->body.driver_name.content, lpDriverName);

    list_node->body.driver_caps = driver_caps;
    list_node->body.hel_caps = hel_caps;

    list_node->body.screen_width = 0;
    list_node->body.hw_render_supported = false;
    list_node->body.sw_windowed_supported = false;
    list_node->body.hw_windowed_supported = false;
    list_node->body.is_vga_mode1_presented = false;
    list_node->body.is_vga_mode2_presented = false;

    Enumerate3DDevices(&list_node->body);

cleanup:
    DDrawRelease();
    return TRUE;
}

bool __cdecl WinVidRegisterGameWindowClass(void)
{
    WNDCLASSEXA wnd_class = {
        .cbSize = sizeof(WNDCLASSEXA),
        .style = 0,
        .lpfnWndProc = WinVidGameWindowProc,
        .hInstance = g_GameModule,
        .hIcon = LoadIcon(g_GameModule, MAKEINTRESOURCE(IDI_MAINICON)),
        .hCursor = LoadCursor(NULL, IDC_ARROW),
        .lpszClassName = g_GameClassName,
        0,
    };
    return RegisterClassExA(&wnd_class) != 0;
}

LRESULT CALLBACK
WinVidGameWindowProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (g_IsFMVPlaying) {
        switch (Msg) {
        case WM_DESTROY:
            g_IsGameWindowCreated = false;
            g_GameWindowHandle = NULL;
            PostQuitMessage(0);
            break;

        case WM_MOVE:
            g_GameWindowPositionX = LOWORD(lParam);
            g_GameWindowPositionY = HIWORD(lParam);
            break;

        case WM_ACTIVATEAPP:
            g_IsGameWindowActive = wParam != 0;
            break;

        case WM_SYSCOMMAND:
            if (wParam == SC_KEYMENU) {
                return 0;
            }
            break;
        }
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }

    switch (Msg) {
    case WM_CREATE:
        g_IsGameWindowCreated = true;
        break;

    case WM_DESTROY:
        g_IsGameWindowCreated = false;
        g_GameWindowHandle = NULL;
        PostQuitMessage(0);
        break;

    case WM_MOVE:
        g_GameWindowPositionX = LOWORD(lParam);
        g_GameWindowPositionY = HIWORD(lParam);
        break;

    case WM_SIZE:
        switch (wParam) {
        case SIZE_RESTORED:
            g_IsGameWindowMinimized = false;
            g_IsGameWindowMaximized = false;
            break;

        case SIZE_MAXIMIZED:
            g_IsGameWindowMinimized = false;
            g_IsGameWindowMaximized = true;
            break;

        case SIZE_MINIMIZED:
            g_IsGameWindowMinimized = true;
            g_IsGameWindowMaximized = false;
            return DefWindowProc(hWnd, Msg, wParam, lParam);

        default:
            return DefWindowProc(hWnd, Msg, wParam, lParam);
        }

        if (g_IsGameFullScreen
            || (LOWORD(lParam) == g_GameWindowWidth
                && HIWORD(lParam) == g_GameWindowHeight)) {
            break;
        }

        g_GameWindowWidth = LOWORD(lParam);
        g_GameWindowHeight = HIWORD(lParam);
        if (g_IsGameWindowUpdating) {
            break;
        }

        UpdateGameResolution();
        break;

    case WM_PAINT: {
        PAINTSTRUCT paint;
        HDC hdc = BeginPaint(hWnd, &paint);
        LPDDS surface = (g_SavedAppSettings.render_mode == RM_SOFTWARE)
            ? g_RenderBufferSurface
            : g_BackBufferSurface;
        if (g_IsGameFullScreen || !g_PrimaryBufferSurface || !surface) {
            HBRUSH brush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            FillRect(hdc, &paint.rcPaint, brush);
        } else {
            if (g_SavedAppSettings.render_mode == RM_SOFTWARE
                && !WinVidCheckGameWindowPalette(hWnd)
                && g_RenderBufferSurface) {
                WinVidClearBuffer(g_RenderBufferSurface, NULL, 0);
            }
            UpdateFrame(false, NULL);
        }
        EndPaint(hWnd, &paint);
        return 0;
    }

    case WM_ACTIVATE:
        if (LOWORD(wParam) && g_DDrawPalette != NULL
            && g_PrimaryBufferSurface != NULL) {
            g_PrimaryBufferSurface->lpVtbl->SetPalette(
                g_PrimaryBufferSurface, g_DDrawPalette);
        }
        break;

    case WM_ERASEBKGND:
        return 1;

    case WM_ACTIVATEAPP:
        if (wParam && !g_IsGameWindowActive && g_IsGameFullScreen
            && g_SavedAppSettings.render_mode == RM_HARDWARE) {
            g_WinVidNeedToResetBuffers = true;
        }
        g_IsGameWindowActive = (wParam != 0);
        break;

    case WM_SETCURSOR:
        if (g_IsGameFullScreen) {
            SetCursor(NULL);
            return 1;
        }
        break;

    case WM_GETMINMAXINFO:
        if (WinVidGetMinMaxInfo((LPMINMAXINFO)lParam)) {
            return 0;
        }
        break;

    case WM_NCPAINT:
    case WM_NCLBUTTONDOWN:
    case WM_NCLBUTTONDBLCLK:
    case WM_NCRBUTTONDOWN:
    case WM_NCRBUTTONDBLCLK:
    case WM_NCMBUTTONDOWN:
    case WM_NCMBUTTONDBLCLK:
        if (g_IsGameFullScreen) {
            return 0;
        }
        break;

    case WM_SYSCOMMAND:
        if (wParam == SC_KEYMENU) {
            return 0;
        }
        break;

    case WM_SIZING:
        WinVidResizeGameWindow(hWnd, wParam, (LPRECT)lParam);
        break;

    case WM_MOVING:
        if (g_IsGameFullScreen || g_IsGameWindowMaximized) {
            GetWindowRect(hWnd, (LPRECT)lParam);
            return 1;
        }
        break;

    case WM_ENTERSIZEMOVE:
        g_IsGameWindowChanging = true;
        break;

    case WM_EXITSIZEMOVE:
        g_IsGameWindowChanging = false;
        break;

    case WM_PALETTECHANGED:
        if (hWnd != (HWND)wParam && !g_IsGameFullScreen && g_DDrawPalette) {
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    }

    return DefWindowProc(hWnd, Msg, wParam, lParam);
}

void __cdecl WinVidResizeGameWindow(HWND hWnd, int32_t edge, LPRECT rect)
{
    if (g_IsGameFullScreen) {
        rect->left = 0;
        rect->top = 0;
        rect->right = g_FullScreenWidth;
        rect->bottom = g_FullScreenHeight;
    }

    const bool is_shift_pressed = GetAsyncKeyState(VK_SHIFT) < 0;
    int32_t width = rect->right - rect->left;
    int32_t height = rect->bottom - rect->top;
    GameWindowCalculateSizeFromClientByZero(&width, &height);

    if (edge == WMSZ_TOP || edge == WMSZ_BOTTOM) {
        if (is_shift_pressed) {
            height &= ~0x1F;
        }
        width = CalculateWindowWidth(width, height);
    } else {
        if (is_shift_pressed) {
            width &= ~0x1F;
        }
        height = CalculateWindowHeight(width, height);
    }

    if (g_IsMinWindowSizeSet) {
        CLAMPL(width, g_MinWindowClientWidth);
        CLAMPL(height, g_MinWindowClientHeight);
    }

    if (g_IsMaxWindowSizeSet) {
        CLAMPG(width, g_MaxWindowClientWidth);
        CLAMPG(height, g_MaxWindowClientHeight);
    }

    GameWindowCalculateSizeFromClient(&width, &height);

    switch (edge) {
    case WMSZ_TOPLEFT:
        rect->left = rect->right - width;
        rect->top = rect->bottom - height;
        break;

    case WMSZ_RIGHT:
    case WMSZ_BOTTOM:
    case WMSZ_BOTTOMRIGHT:
        rect->right = rect->left + width;
        rect->bottom = rect->top + height;
        break;

    case WMSZ_LEFT:
    case WMSZ_BOTTOMLEFT:
        rect->left = rect->right - width;
        rect->bottom = rect->top + height;
        break;

    case WMSZ_TOP:
    case WMSZ_TOPRIGHT:
        rect->right = rect->left + width;
        rect->top = rect->bottom - height;
        break;
    }
}

bool __cdecl WinVidCheckGameWindowPalette(HWND hWnd)
{
    const HDC hdc = GetDC(hWnd);
    if (hdc == NULL) {
        return false;
    }

    PALETTEENTRY sys_palette[256];
    GetSystemPaletteEntries(hdc, 0, 256, sys_palette);
    ReleaseDC(hWnd, hdc);

    RGB_888 buf_palette[256];
    for (int32_t i = 0; i < 256; i++) {
        buf_palette[i].red = sys_palette[i].peRed;
        buf_palette[i].green = sys_palette[i].peGreen;
        buf_palette[i].blue = sys_palette[i].peBlue;
    }

    return memcmp(buf_palette, g_GamePalette8, sizeof(buf_palette)) == 0;
}

bool __cdecl WinVidCreateGameWindow(void)
{
    g_IsGameWindowActive = true;
    g_IsGameWindowShow = true;
    g_IsDDrawGameWindowShow = false;
    g_IsMessageLoopClosed = false;
    g_IsGameWindowUpdating = false;
    g_IsGameWindowMinimized = false;
    g_IsGameWindowMaximized = false;
    g_IsGameWindowCreated = false;
    g_IsGameFullScreen = false;
    g_IsMinMaxInfoSpecial = false;
    g_IsGameWindowChanging = false;

    WinVidClearMinWindowSize();
    WinVidClearMaxWindowSize();

    g_GameWindowHandle = CreateWindowEx(
        WS_EX_APPWINDOW, g_GameClassName, g_GameWindowName, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL,
        g_GameModule, NULL);
    if (g_GameWindowHandle == NULL) {
        return false;
    }

    RECT rect;
    GetWindowRect(g_GameWindowHandle, &rect);
    g_GameWindowX = rect.left;
    g_GameWindowY = rect.top;

    WinVidHideGameWindow();

    return true;
}

void __cdecl WinVidFreeWindow(void)
{
    WinVidExitMessage();
    UnregisterClassA(g_GameClassName, g_GameModule);
}

void __cdecl WinVidExitMessage(void)
{
    if (g_GameWindowHandle == NULL || !IsWindow(g_GameWindowHandle)) {
        return;
    }

    PostMessage(g_GameWindowHandle, WM_CLOSE, 0, 0);
    while (WinVidSpinMessageLoop(false)) { }
    g_GameWindowHandle = NULL;
}

DISPLAY_ADAPTER_NODE *__cdecl WinVidGetDisplayAdapter(const GUID *guid_ptr)
{
    if (guid_ptr != NULL) {
        for (DISPLAY_ADAPTER_NODE *adapter = g_DisplayAdapterList.head;
             adapter != NULL; adapter = adapter->next) {
            if (memcmp(&adapter->body.adapter_guid, guid_ptr, sizeof(GUID))
                == 0) {
                return adapter;
            }
        }
    }
    return g_PrimaryDisplayAdapter;
}

void __cdecl WinVidStart(void)
{
    if (g_SavedAppSettings.preferred_display_adapter == NULL) {
        Shell_ExitSystem("Can't create DirectDraw");
    }

    DISPLAY_ADAPTER *preferred =
        &g_SavedAppSettings.preferred_display_adapter->body;
    g_CurrentDisplayAdapter = *preferred;

    S_FlaggedString_Copy(
        &g_CurrentDisplayAdapter.driver_desc, &preferred->driver_desc);
    S_FlaggedString_Copy(
        &g_CurrentDisplayAdapter.driver_name, &preferred->driver_name);

    M_DisplayModeListInit(&g_CurrentDisplayAdapter.hw_disp_mode_list);
    M_DisplayModeListCopy(
        &g_CurrentDisplayAdapter.hw_disp_mode_list,
        &preferred->hw_disp_mode_list);

    M_DisplayModeListInit(&g_CurrentDisplayAdapter.sw_disp_mode_list);
    M_DisplayModeListCopy(
        &g_CurrentDisplayAdapter.sw_disp_mode_list,
        &preferred->sw_disp_mode_list);

    if (!DDrawCreate(g_CurrentDisplayAdapter.adapter_guid_ptr)) {
        Shell_ExitSystem("Can't create DirectDraw");
    }
}

void __cdecl WinVidFinish(void)
{
    if (g_IsDDrawGameWindowShow) {
        HideDDrawGameWindow();
    }
    DDrawRelease();
}

int32_t __cdecl Misc_Move3DPosTo3DPos(
    PHD_3DPOS *const src_pos, const PHD_3DPOS *const dst_pos,
    const int32_t velocity, const PHD_ANGLE ang_add)
{
    // TODO: this function's only usage is in Lara_MovePosition. inline it
    const XYZ_32 dpos = {
        .x = dst_pos->pos.x - src_pos->pos.x,
        .y = dst_pos->pos.y - src_pos->pos.y,
        .z = dst_pos->pos.z - src_pos->pos.z,
    };
    const int32_t dist = XYZ_32_GetDistance0(&dpos);
    if (velocity >= dist) {
        src_pos->pos.x = dst_pos->pos.x;
        src_pos->pos.y = dst_pos->pos.y;
        src_pos->pos.z = dst_pos->pos.z;
    } else {
        src_pos->pos.x += velocity * dpos.x / dist;
        src_pos->pos.y += velocity * dpos.y / dist;
        src_pos->pos.z += velocity * dpos.z / dist;
    }

#define ADJUST_ROT(source, target, rot)                                        \
    do {                                                                       \
        if ((PHD_ANGLE)(target - source) > rot) {                              \
            source += rot;                                                     \
        } else if ((PHD_ANGLE)(target - source) < -rot) {                      \
            source -= rot;                                                     \
        } else {                                                               \
            source = target;                                                   \
        }                                                                      \
    } while (0)

    ADJUST_ROT(src_pos->rot.x, dst_pos->rot.x, ang_add);
    ADJUST_ROT(src_pos->rot.y, dst_pos->rot.y, ang_add);
    ADJUST_ROT(src_pos->rot.z, dst_pos->rot.z, ang_add);

    // clang-format off
    return (
        src_pos->pos.x == dst_pos->pos.x &&
        src_pos->pos.y == dst_pos->pos.y &&
        src_pos->pos.z == dst_pos->pos.z &&
        src_pos->rot.x == dst_pos->rot.x &&
        src_pos->rot.y == dst_pos->rot.y &&
        src_pos->rot.z == dst_pos->rot.z
    );
    // clang-format on
}

int32_t __cdecl LevelCompleteSequence(void)
{
    return GFD_EXIT_TO_TITLE;
}

void __cdecl S_LoadSettings(void)
{
    OpenGameRegistryKey("Game");

    {
        DWORD tmp;
        GetRegistryDwordValue("MusicVolume", &tmp, 165);
        g_OptionMusicVolume = tmp;
    }

    {
        DWORD tmp;
        GetRegistryDwordValue("SoundFXVolume", &tmp, 10);
        g_OptionSoundVolume = tmp;
    }

    {
        DWORD tmp;
        GetRegistryDwordValue("DetailLevel", &tmp, 1);
        g_DetailLevel = tmp;
    }

    GetRegistryFloatValue("Sizer", &g_GameSizerCopy, 1.0);
    GetRegistryBinaryValue(
        "Layout", (uint8_t *)&g_Layout[1],
        sizeof(uint16_t) * INPUT_ROLE_NUMBER_OF, 0);

    CloseGameRegistryKey();

    Input_CheckConflictsWithDefaults();

    Sound_SetMasterVolume(6 * g_OptionSoundVolume + 4);

    if (g_OptionMusicVolume) {
        Music_SetVolume(25 * g_OptionMusicVolume + 5);
    } else {
        Music_SetVolume(0);
    }
}

void __cdecl S_SaveSettings(void)
{
    OpenGameRegistryKey("Game");
    SetRegistryDwordValue("MusicVolume", g_OptionMusicVolume);
    SetRegistryDwordValue("SoundFxVolume", g_OptionSoundVolume);
    SetRegistryDwordValue("DetailLevel", g_DetailLevel);
    SetRegistryFloatValue("Sizer", g_GameSizerCopy);
    SetRegistryBinaryValue(
        "Layout", (uint8_t *)&g_Layout[1],
        sizeof(uint16_t) * INPUT_ROLE_NUMBER_OF);
    CloseGameRegistryKey();
}
