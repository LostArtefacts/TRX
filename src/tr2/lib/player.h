static int(__cdecl *Movie_GetCurrentFrame)(LPVOID);
static int(__cdecl *Movie_GetFormat)(LPVOID);
static int(__cdecl *Movie_GetSoundChannels)(LPVOID);
static int(__cdecl *Movie_GetSoundPrecision)(LPVOID);
static int(__cdecl *Movie_GetSoundRate)(LPVOID);
static int(__cdecl *Movie_GetTotalFrames)(LPVOID);
static int(__cdecl *Movie_GetXSize)(LPVOID);
static int(__cdecl *Movie_GetYSize)(LPVOID);
static int(__cdecl *Movie_SetSyncAdjust)(LPVOID, LPVOID, DWORD);
static int(__cdecl *Player_BlankScreen)(DWORD, DWORD, DWORD, DWORD);
static int(__cdecl *Player_GetDSErrorCode)();
static int(__cdecl *Player_InitMovie)(LPVOID, DWORD, DWORD, LPCTSTR, DWORD);
static int(__cdecl *Player_InitMoviePlayback)(LPVOID, LPVOID, LPVOID);
static int(__cdecl *Player_InitPlaybackMode)(HWND, LPVOID, DWORD, DWORD);
static int(__cdecl *Player_InitSound)(
    LPVOID, DWORD, DWORD, BOOL, DWORD, DWORD, DWORD, DWORD, DWORD);
static int(__cdecl *Player_InitSoundSystem)(HWND);
static int(__cdecl *Player_InitVideo)(
    LPVOID, LPVOID, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD,
    DWORD, DWORD, DWORD);
static int(__cdecl *Player_PassInDirectDrawObject)(LPDIRECTDRAW3);
static int(__cdecl *Player_PlayFrame)(
    LPVOID, LPVOID, LPVOID, DWORD, LPRECT, DWORD, DWORD, DWORD);
static int(__cdecl *Player_ReturnPlaybackMode)(BOOL);
static int(__cdecl *Player_ShutDownMovie)(LPVOID);
static int(__cdecl *Player_ShutDownSound)(LPVOID);
static int(__cdecl *Player_ShutDownSoundSystem)();
static int(__cdecl *Player_ShutDownVideo)(LPVOID);
static int(__cdecl *Player_StartTimer)(LPVOID);
static int(__cdecl *Player_StopTimer)(LPVOID);
