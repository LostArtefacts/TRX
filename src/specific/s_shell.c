#include "specific/s_shell.h"

#include "config.h"
#include "game/game.h"
#include "game/gamebuf.h"
#include "game/music.h"
#include "game/shell.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "inject_util.h"
#include "log.h"
#include "specific/s_ati.h"
#include "specific/s_clock.h"
#include "specific/s_hwr.h"
#include "specific/s_input.h"

#include <ddraw.h>
#include <dinput.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <windows.h>

static const char *ClassName = "TRClass";
static const char *WindowName = "Tomb Raider";
static UINT CloseMsg = 0;
static bool IsGameWindowActive = true;

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static bool S_Shell_InitDirectDraw();
static void S_Shell_TerminateGame(int exit_code);
static void S_Shell_ShowFatalError(const char *message);

void S_Shell_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    SeedRandomControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    SeedRandomDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

static bool S_Shell_InitDirectDraw()
{
    if (DirectDrawCreate(0, &DDraw, 0)) {
        S_Shell_ShowFatalError("DirectDraw could not be started");
        return false;
    }

    if (S_ATI_Init()) {
        S_Shell_ShowFatalError("ATI3DCIF could not be started");
        return false;
    }

    return true;
}

static void S_Shell_TerminateGame(int exit_code)
{
    if (DDraw) {
        HWR_ReleaseSurfaces();
        IDirectDraw_FlipToGDISurface(DDraw);
        IDirectDraw_FlipToGDISurface(DDraw);
        IDirectDraw_RestoreDisplayMode(DDraw);
        IDirectDraw_SetCooperativeLevel(DDraw, TombHWND, 8);
        IDirectDraw_Release(DDraw);
        DDraw = NULL;
    }

    S_ATI_Shutdown();

    PostMessageA(HWND_BROADCAST, CloseMsg, 0, 0);
    exit(exit_code);
}

static void S_Shell_ShowFatalError(const char *message)
{
    LOG_ERROR("%s", message);
    MessageBoxA(
        0, message, "Tomb Raider Error", MB_SETFOREGROUND | MB_ICONEXCLAMATION);
    S_Shell_TerminateGame(1);
}

void S_Shell_SpinMessageLoop()
{
    MSG msg;
    do {
        while (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE)) {
            if (!GetMessageA(&msg, 0, 0, 0)) {
                S_Shell_TerminateGame(0);
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    } while (!IsGameWindowActive);
}

static LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == CloseMsg) {
        DestroyWindow(TombHWND);
        return 0;
    }

    switch (uMsg) {
    case WM_CLOSE:
        DestroyWindow(TombHWND);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        return 1;

    case WM_MOVE:
        return 0;

    case WM_SETCURSOR:
        SetCursor(0);
        return 1;

    case WM_ERASEBKGND:
        return 1;

    case WM_ACTIVATEAPP:
        // mute the music when the game is not active
        if (!IsGameWindowActive && wParam) {
            Music_SetVolume(T1MConfig.music_volume);
        } else if (IsGameWindowActive && !wParam) {
            Music_SetVolume(0);
        }
        IsGameWindowActive = wParam != 0;
        return 1;

    case WM_NCPAINT:
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO *min_max_info = (MINMAXINFO *)lParam;
        min_max_info->ptMinTrackSize.x = DDrawSurfaceWidth;
        min_max_info->ptMinTrackSize.y = DDrawSurfaceHeight;
        min_max_info->ptMaxTrackSize.x = DDrawSurfaceWidth;
        min_max_info->ptMaxTrackSize.y = DDrawSurfaceHeight;
        return DefWindowProcA(
            hWnd, WM_GETMINMAXINFO, wParam, (LPARAM)min_max_info);
    }

    case WM_MOVING:
        GetWindowRect(TombHWND, (LPRECT)lParam);
        return 1;

    case WM_SYSCOMMAND:
        // ignore alt keypresses to bring window menu
        if (wParam == SC_KEYMENU && (lParam >> 16) <= 0) {
            return 0;
        }
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    case MM_MCINOTIFY:
        Music_PlayLooped();
        return 0;

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSA WndClass = { 0 };
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIconA(hInstance, IDI_APPLICATION);
    WndClass.hCursor = LoadCursorA(0, IDC_ARROW);
    WndClass.lpszMenuName = 0;
    WndClass.lpszClassName = ClassName;
    RegisterClassA(&WndClass);

    int32_t scr_height = GetSystemMetrics(SM_CYSCREEN);
    int32_t scr_width = GetSystemMetrics(SM_CXSCREEN);

    TombModule = hInstance;
    TombHWND = CreateWindowExA(
        0, ClassName, WindowName, WS_VISIBLE | WS_POPUP | WS_SYSMENU, 0, 0,
        scr_width, scr_height, 0, 0, hInstance, 0);

    if (!TombHWND) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    SetWindowPos(TombHWND, 0, 0, 0, scr_width, scr_height, SWP_NOCOPYBITS);
    ShowWindow(TombHWND, nShowCmd);
    UpdateWindow(TombHWND);
    CloseMsg = RegisterWindowMessageA("CLOSE_HACK");

    if (!S_Shell_InitDirectDraw()) {
        S_Shell_TerminateGame(1);
        return 1;
    }

    Shell_Main();

    S_Shell_TerminateGame(0);
    return 0;
}

void S_Shell_ExitSystem(const char *message)
{
    while (Input.select) {
        S_UpdateInput();
    }
    GameBuf_Shutdown();
    HWR_ShutdownHardware();
    S_Shell_ShowFatalError(message);
}

void S_Shell_ExitSystemFmt(const char *fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    char message[150];
    vsnprintf(message, 150, fmt, va);
    va_end(va);
    S_Shell_ExitSystem(message);
}

void S_Shell_Inject()
{
    INJECT(0x0043DA80, WinMain);
    INJECT(0x0043DE00, WndProc);
}
