#include "specific/smain.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/ati.h"
#include "specific/clock.h"
#include "specific/hwr.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/shell.h"
#include "specific/sndpc.h"
#include "util.h"

#include <windows.h>
#include <ddraw.h>
#include <dinput.h>

static const char *ClassName = "TRClass";
static const char *WindowName = "Tomb Raider";

// clang-format off
// TODO: decompile me!
#define dword_45A990            VAR_U_(0x0045A990, int32_t)
#define CloseMsg                VAR_U_(0x0045A940, UINT)
// clang-format on

static void WinGameFinish();
static LRESULT CALLBACK
WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static int InitDirectDraw();

void TerminateGame(int exit_code)
{
    WinGameFinish();
    exit(exit_code);
}

void ShowFatalError(const char *message)
{
    LOG_ERROR("%s", message);
    MessageBoxA(
        0, message, "Tomb Raider Error", MB_SETFOREGROUND | MB_ICONEXCLAMATION);
    TerminateGame(1);
}

void WinSpinMessageLoop()
{
    MSG msg;
    do {
        while (PeekMessageA(&msg, 0, 0, 0, PM_NOREMOVE)) {
            if (!GetMessageA(&msg, 0, 0, 0)) {
                TerminateGame(0);
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
    } while (!IsGameWindowActive);
}

static int InitDirectDraw()
{
    if (DirectDrawCreate(0, &DDraw, 0)) {
        ShowFatalError("DirectDraw could not be started");
        return 0;
    }

    if (InitATI3DCIF()) {
        ShowFatalError("ATI3DCIF could not be started");
        return 0;
    }

    ATIRenderContext = ATI3DCIF_ContextCreate();
    if (!ATIRenderContext) {
        ShowFatalError("ATI3DCIF could not be started");
        return 0;
    }

    ATIInfo.u32Size = sizeof(C3D_3DCIFINFO);
    if (ATI3DCIF_GetInfo(&ATIInfo)) {
        ShowFatalError("Failed to parse ATI3DCIF capabilities");
        return 0;
    }

    if (!(ATIInfo.u32CIFCaps1 & C3D_CAPS1_Z_BUFFER)) {
        ShowFatalError("Z-Buffer capability not found");
        return 0;
    }

    if (!(ATIInfo.u32CIFCaps1 & C3D_CAPS1_CI8_TMAP)) {
        ShowFatalError("8-bit texture capability not found");
        return 0;
    }

    return 1;
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
        if (wParam && !IsGameWindowActive) {
            if (OptionMusicVolume) {
                S_MusicVolume(OptionMusicVolume * 25 + 5);
            } else {
                S_MusicVolume(0);
            }
        } else if (!wParam && IsGameWindowActive) {
            S_MusicVolume(0);
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
        MusicPlayLooped();
        return 0;

    default:
        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }
}

static void WinGameFinish()
{
    dword_45A990 = 1;
    if (DDraw) {
        HWR_ReleaseSurfaces();
        IDirectDraw_FlipToGDISurface(DDraw);
        IDirectDraw_FlipToGDISurface(DDraw);
        IDirectDraw_RestoreDisplayMode(DDraw);
        IDirectDraw_SetCooperativeLevel(DDraw, TombHWND, 8);
        IDirectDraw_Release(DDraw);
        DDraw = NULL;
    }
    if (ATIRenderContext) {
        ATI3DCIF_ContextDestroy(ATIRenderContext);
        ShutdownATI3DCIF();
        ATIRenderContext = 0;
    }
    PostMessageA(HWND_BROADCAST, CloseMsg, 0, 0);
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
        ShowFatalError("System Error: cannot create window");
        return 1;
    }

    SetWindowPos(TombHWND, 0, 0, 0, scr_width, scr_height, SWP_NOCOPYBITS);
    ShowWindow(TombHWND, nShowCmd);
    UpdateWindow(TombHWND);
    CloseMsg = RegisterWindowMessageA("CLOSE_HACK");

    if (!InitDirectDraw()) {
        WinGameFinish();
        exit(1);
        return 1;
    }

    GameMain();

    WinGameFinish();
    exit(0);
    return 0;
}

void T1MInjectSpecificSMain()
{
    INJECT(0x0043D070, InitDirectDraw);
    INJECT(0x0043D510, TerminateGame);
    INJECT(0x0043D770, ShowFatalError);
    INJECT(0x0043DA80, WinMain);
    INJECT(0x0043DE00, WndProc);
}