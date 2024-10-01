#pragma once

#include <windows.h>

extern MMRESULT(__stdcall *g_MM_auxGetDevCapsA)(
    UINT_PTR uDeviceID, LPAUXCAPSA pac, UINT cbac);

extern UINT(__stdcall *g_MM_auxGetNumDevs)(void);

extern MMRESULT(__stdcall *g_MM_auxSetVolume)(UINT uDeviceID, DWORD dwVolume);

extern MCIERROR(__stdcall *g_MM_mciSendCommandA)(
    MCIDEVICEID mciId, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

extern MCIERROR(__stdcall *g_MM_mciSendStringA)(
    LPCSTR lpszCommand, LPSTR lpszReturnString, UINT cchReturn,
    HANDLE hwndCallback);

void WinMM_Load(void);

#define auxGetDevCapsA g_MM_auxGetDevCapsA
#define auxGetNumDevs g_MM_auxGetNumDevs
#define auxSetVolume g_MM_auxSetVolume
#define mciSendCommandA g_MM_mciSendCommandA
#define mciSendStringA g_MM_mciSendStringA
