#include "specific/smain.h"

#include "global/vars_platform.h"
#include "specific/input.h"
#include "util.h"

#include <windows.h>

LRESULT KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    TRACE("");
    if ( code < 0 ) {
        return CallNextHookEx(HHK, code, wParam, lParam);
    }
    if ( wParam == 145 ) {
        PostMessageA(TombHWND, 0x10u, 0, 0);
    }
    OnKeyPress(lParam & 0x1000000, (lParam >> 16) & 0xFF, lParam >= 0);
    if ( wParam < 0x11 || wParam > 0x12 ) {
        return CallNextHookEx(HHK, code, wParam, lParam);
    }
    return 1;
}

void T1MInjectSpecificSMain()
{
    INJECT(0x0043D8C0, KeyboardHook);
}
