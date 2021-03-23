#include "specific/smain.h"

#include "global/vars.h"
#include "global/vars_platform.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/shell.h"
#include "util.h"

#include <windows.h>
#include <dinput.h>

static const char *ClassName = "TRClass";
static const char *WindowName = "Tomb Raider";

#pragma pack(push, 1)
// TODO: decompile me!
typedef struct UNK1 {
    char tmp0[8];
    void(__stdcall *cb8)(struct UNK1 **);
    char tmpC[28];
    void(__stdcall *cb28)(struct UNK1 **);
    char tmp2C[32];
    void(__stdcall *cb4C)(struct UNK1 **);
    void(__stdcall *cb50)(struct UNK1 **, HWND, int);
} UNK1;

// TODO: decompile me!
typedef struct UNK2 {
    char tmp0[8];
    void(__stdcall *cb8)(struct UNK2 **);
} UNK2;
#pragma pack(pop)

// clang-format off
// TODO: decompile me!
#define sub_43D070      ((int (*)())0x0043D070)
#define sub_4508C0      ((void (*)())0x004508C0)
#define sub_407A91      ((void (*)())0x00407A91)
#define sub_450830      ((void (*)(int32_t))0x00450830)
#define WndProc         ((LRESULT __stdcall (*)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam))0x0043DE00)
// clang-format off

// clang-format off
// TODO: decompile me!
#define dword_45A938    VAR_U_(0x0045A938, UNK2**)
#define dword_45A998    VAR_U_(0x0045A998, UNK1**)
#define dword_45A994    VAR_U_(0x0045A994, int32_t)
#define dword_45A990    VAR_U_(0x0045A990, int32_t)
#define Msg             VAR_U_(0x0045A940, UINT)
// clang-format on

static void WinGameFinish();
static LRESULT WINAPI KeyboardHook(int code, WPARAM wParam, LPARAM lParam);

void ShowFatalError(const char *message)
{
    DB_Log(message);
    if (dword_45A938) {
        (*dword_45A938)->cb8(dword_45A938);
        dword_45A938 = 0;
    }
    if (dword_45A998) {
        sub_407A91();
        (*dword_45A998)->cb28(dword_45A998);
        (*dword_45A998)->cb4C(dword_45A998);
        (*dword_45A998)->cb50(dword_45A998, TombHWND, 8);
        (*dword_45A998)->cb8(dword_45A998);
        dword_45A998 = 0;
    }
    MessageBoxA(
        0, message, "Tomb Raider Error", MB_SETFOREGROUND | MB_ICONEXCLAMATION);
    WinGameFinish();
    exit(1);
}

static LRESULT WINAPI KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    if (code < 0) {
        return CallNextHookEx(HHK, code, wParam, lParam);
    }

    uint32_t keyData = (uint32_t)lParam;
    uint32_t scan_code = (keyData >> 16) & 0xFF;
    uint32_t extended = (keyData >> 24) & 0x1;
    uint32_t pressed = ~keyData >> 31;

    if (scan_code == DIK_LCONTROL) {
        scan_code = DIK_RCONTROL;
    } else if (scan_code == DIK_LSHIFT) {
        scan_code = DIK_RSHIFT;
    } else if (scan_code == DIK_LMENU) {
        scan_code = DIK_RMENU;
    } else if (extended) {
        scan_code += 128;
    }

    if (!KeyData) {
        return CallNextHookEx(HHK, code, wParam, lParam);
    }

    KeyData->keymap[scan_code] = pressed;
    KeyData->keys_held = pressed;

    return wParam == VK_MENU;
}

static void WinGameFinish()
{
    DB_Log("TerminateGame");
    dword_45A990 = 1;
    DB_Log("Shutting down DirectDraw");
    if (dword_45A938) {
        (*dword_45A938)->cb8(dword_45A938);
        dword_45A938 = 0;
    }
    if (dword_45A998) {
        sub_407A91();
        (*dword_45A998)->cb28(dword_45A998);
        (*dword_45A998)->cb4C(dword_45A998);
        (*dword_45A998)->cb50(dword_45A998, TombHWND, 8);
        (*dword_45A998)->cb8(dword_45A998);
        dword_45A998 = 0;
    }
    if (dword_45A994) {
        DB_Log("TerminateLibrary");
        sub_450830(dword_45A994);
        sub_4508C0();
        dword_45A994 = 0;
    }
    PostMessageA(HWND_BROADCAST, Msg, 0, 0);
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
    Msg = RegisterWindowMessageA("CLOSE_HACK");
    HHK =
        SetWindowsHookExA(WH_KEYBOARD, &KeyboardHook, 0, GetCurrentThreadId());

    DB_Log("StartTombRaider: running game");
    if (!sub_43D070()) {
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
    INJECT(0x0043D770, ShowFatalError);
    INJECT(0x0043D8C0, KeyboardHook);
    INJECT(0x0043DA80, WinMain);
}
