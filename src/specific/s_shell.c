#include "specific/s_shell.h"

#include "config.h"
#include "game/game.h"
#include "game/gamebuf.h"
#include "game/input.h"
#include "game/music.h"
#include "game/random.h"
#include "game/shell.h"
#include "global/vars.h"
#include "global/vars_platform.h"
#include "log.h"
#include "memory.h"
#include "specific/s_hwr.h"
#include "specific/s_input.h"

#include <ddraw.h>
#include <shellapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <windows.h>

static const char *m_ClassName = "TRClass";
static const char *m_WindowName = "Tomb Raider";
static UINT m_CloseMsg = 0;
static bool m_IsGameWindowActive = true;

static LRESULT CALLBACK
m_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HMODULE m_DDraw = NULL;
HRESULT (*m_DirectDrawCreate)(GUID *, LPDIRECTDRAW *, IUnknown *) = NULL;

static bool S_Shell_InitDirectDraw();
static void S_Shell_TerminateGame(int exit_code);
static void S_Shell_ShowFatalError(const char *message);

void S_Shell_SeedRandom()
{
    time_t lt = time(0);
    struct tm *tptr = localtime(&lt);
    Random_SeedControl(tptr->tm_sec + 57 * tptr->tm_min + 3543 * tptr->tm_hour);
    Random_SeedDraw(tptr->tm_sec + 43 * tptr->tm_min + 3477 * tptr->tm_hour);
}

static bool S_Shell_InitDirectDraw()
{
    if (!g_GLRage) {
        g_GLRage = LoadLibrary("glrage.dll");
    }

    m_DDraw = g_GLRage;
    if (!m_DDraw) {
        S_Shell_ShowFatalError("Cannot find glrage.dll");
        return false;
    }

    m_DirectDrawCreate =
        (HRESULT(*)(GUID *, LPDIRECTDRAW *, IUnknown *))GetProcAddress(
            m_DDraw, "DirectDrawCreate");
    if (!m_DirectDrawCreate) {
        S_Shell_ShowFatalError("Cannot find DirectDrawCreate");
        return false;
    }

    if (m_DirectDrawCreate(NULL, &g_DDraw, NULL)) {
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
    if (g_DDraw) {
        HWR_ReleaseSurfaces();
        IDirectDraw_FlipToGDISurface(g_DDraw);
        IDirectDraw_FlipToGDISurface(g_DDraw);
        IDirectDraw_RestoreDisplayMode(g_DDraw);
        IDirectDraw_SetCooperativeLevel(g_DDraw, g_TombHWND, 8);
        IDirectDraw_Release(g_DDraw);
        g_DDraw = NULL;
    }

    S_ATI_Shutdown();

    PostMessageA(HWND_BROADCAST, m_CloseMsg, 0, 0);
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
    } while (!m_IsGameWindowActive);
}

static LRESULT CALLBACK
m_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == m_CloseMsg) {
        DestroyWindow(g_TombHWND);
        return 0;
    }

    switch (uMsg) {
    case WM_CLOSE:
        DestroyWindow(g_TombHWND);
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
        if (!m_IsGameWindowActive && wParam) {
            Music_SetVolume(g_Config.music_volume);
        } else if (m_IsGameWindowActive && !wParam) {
            Music_SetVolume(0);
        }
        m_IsGameWindowActive = wParam != 0;
        return 1;

    case WM_NCPAINT:
        return 0;

    case WM_GETMINMAXINFO: {
        MINMAXINFO *min_max_info = (MINMAXINFO *)lParam;
        min_max_info->ptMinTrackSize.x = g_DDrawSurfaceWidth;
        min_max_info->ptMinTrackSize.y = g_DDrawSurfaceHeight;
        min_max_info->ptMaxTrackSize.x = g_DDrawSurfaceWidth;
        min_max_info->ptMaxTrackSize.y = g_DDrawSurfaceHeight;
        return DefWindowProcA(
            hWnd, WM_GETMINMAXINFO, wParam, (LPARAM)min_max_info);
    }

    case WM_MOVING:
        GetWindowRect(g_TombHWND, (LPRECT)lParam);
        return 1;

    case WM_SYSCOMMAND:
        // ignore alt keypresses to bring window menu
        if (wParam == SC_KEYMENU && (lParam >> 16) <= 0) {
            return 0;
        }
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(
    HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASSA WndClass = { 0 };
    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.lpfnWndProc = (WNDPROC)m_WndProc;
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.hInstance = hInstance;
    WndClass.hIcon = LoadIconA(hInstance, IDI_APPLICATION);
    WndClass.hCursor = LoadCursorA(0, IDC_ARROW);
    WndClass.lpszMenuName = 0;
    WndClass.lpszClassName = m_ClassName;
    RegisterClassA(&WndClass);

    int32_t scr_height = GetSystemMetrics(SM_CYSCREEN);
    int32_t scr_width = GetSystemMetrics(SM_CXSCREEN);

    g_TombModule = hInstance;
    g_TombHWND = CreateWindowExA(
        0, m_ClassName, m_WindowName, WS_VISIBLE | WS_POPUP | WS_SYSMENU, 0, 0,
        scr_width, scr_height, 0, 0, hInstance, 0);

    if (!g_TombHWND) {
        S_Shell_ShowFatalError("System Error: cannot create window");
        return 1;
    }

    SetWindowPos(g_TombHWND, 0, 0, 0, scr_width, scr_height, SWP_NOCOPYBITS);
    ShowWindow(g_TombHWND, nShowCmd);
    UpdateWindow(g_TombHWND);
    m_CloseMsg = RegisterWindowMessageA("CLOSE_HACK");

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
    while (g_Input.select) {
        Input_Update();
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

int S_Shell_GetCommandLine(char ***args, int *arg_count)
{
    LPWSTR *l_arg_list;
    int l_arg_count;

    l_arg_list = CommandLineToArgvW(GetCommandLineW(), &l_arg_count);
    if (!l_arg_list) {
        LOG_ERROR("CommandLineToArgvW failed");
        return 0;
    }

    *args = Memory_Alloc(l_arg_count * sizeof(char **));
    *arg_count = l_arg_count;
    for (int i = 0; i < l_arg_count; i++) {
        size_t size = wcslen(l_arg_list[i]) + 1;
        (*args)[i] = Memory_Alloc(size);
        wcstombs((*args)[i], l_arg_list[i], size);
    }

    LocalFree(l_arg_list);

    return 1;
}
