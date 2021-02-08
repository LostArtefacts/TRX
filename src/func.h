#ifndef TR1MAIN_FUNC_H
#define TR1MAIN_FUNC_H

#include <stdio.h>
#include "util.h"

#define game_malloc             ((void      __cdecl*(*)(int, int))0x0041E2F0)
#define ins_line                ((int       __cdecl(*)(int, int, int, int, int, char))0x00402710)
#define _fread                  ((size_t    __cdecl(*)(void *, size_t, size_t, FILE *))0x00442C20)
#define InitialiseItemArray     ((void      __cdecl(*)(int itemCount))0x00421B10)
#define S_ExitSystem            ((void      __cdecl(*)(const char *))0x0041E260)
#define InitialiseItem          ((void      __cdecl(*)(__int16))0x00421CC0)
#define InitialiseLaraInventory ((void      __cdecl(*)(int levelID))0x00428170)
#define InitialiseLOT           ((void      __cdecl(*)())0x0042A780)
#define LoadLevel               ((int       __cdecl(*)(const char *path, int levelID))0x0041AFB0)
#define T_DrawText              ((void      __cdecl(*)())0x00439B00)
#define S_DumpScreen            ((__int32   __cdecl(*)())0x0042FC70)
#define S_InitialisePolyList    ((void      __cdecl(*)())0x0042FC60)
#define S_UpdateInput           ((void      __cdecl(*)())0x0041E550)
#define S_CopyBufferToScreen    ((void      __cdecl(*)())0x00416A60)
#define S_OutputPolyList        ((void      __cdecl(*)())0x0042FD10)
#define sub_41CD10              ((void      __cdecl(*)())0x0041CD10)
#define sub_434520              ((void      __cdecl(*)(int))0x00434520)
#define CreateStartInfo         ((void      __cdecl(*)(int levelID))0x004345E0)
#define TempVideoRemove         ((void      __cdecl(*)())0x004167D0)
#define T_InitPrint             ((void      __cdecl(*)())0x00439750)
#define T_Print                 ((int*      __cdecl(*)(__int16, __int16, __int16, const char *))0x00439780)
#define T_CentreH               ((unsigned int* __cdecl(*)(unsigned int*, __int16))0x004399A0)
#define T_CentreV               ((unsigned int* __cdecl(*)(unsigned int*, __int16))0x004399C0)
#define TempVideoAdjust         ((void      __cdecl(*)(int hi_res, double sizer))0x00416550)

void __cdecl init_game_malloc();
void __cdecl game_free(int freeSize);
void __cdecl DB_Log(char *a1, ...);
const char* __cdecl GetFullPath(const char *filename);
int __cdecl FindCdDrive();
int __cdecl LoadRooms(FILE *fp);
void __cdecl LevelStats(int levelID);
int __cdecl LoadLevelByID(int levelID);
int __cdecl S_DrawHealthBar(int percent);
int __cdecl LoadItems(FILE *handle);
void __cdecl InitialiseLara();

#endif
