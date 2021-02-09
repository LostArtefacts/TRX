#ifndef TR1MAIN_FUNC_H
#define TR1MAIN_FUNC_H

#include <stdio.h>
#include "util.h"

#define game_malloc             ((void          __cdecl*(*)(uint32_t length, int type))0x0041E2F0)
#define ins_line                ((int           __cdecl(*)(int, int, int, int, int, char))0x00402710)
#define _fread                  ((size_t        __cdecl(*)(void *, size_t, size_t, FILE *))0x00442C20)
#define InitialiseItemArray     ((void          __cdecl(*)(int item_count))0x00421B10)
#define S_ExitSystem            ((void          __cdecl(*)(const char *message))0x0041E260)
#define InitialiseItem          ((void          __cdecl(*)(int16_t item_num))0x00421CC0)
#define InitialiseLaraInventory ((void          __cdecl(*)(int level_id))0x00428170)
#define InitialiseLOT           ((void          __cdecl(*)())0x0042A780)
#define LoadLevel               ((int           __cdecl(*)(const char *path, int level_id))0x0041AFB0)
#define T_DrawText              ((void          __cdecl(*)())0x00439B00)
#define S_DumpScreen            ((__int32       __cdecl(*)())0x0042FC70)
#define S_InitialisePolyList    ((void          __cdecl(*)())0x0042FC60)
#define S_UpdateInput           ((void          __cdecl(*)())0x0041E550)
#define S_CopyBufferToScreen    ((void          __cdecl(*)())0x00416A60)
#define S_OutputPolyList        ((void          __cdecl(*)())0x0042FD10)
#define S_FadeToBlack           ((void          __cdecl(*)())0x0041CD10)
#define CreateStartInfo         ((void          __cdecl(*)(int level_id))0x004345E0)
#define ModifyStartInfo         ((void          __cdecl(*)(int level_id))0x00434520)
#define TempVideoRemove         ((void          __cdecl(*)())0x004167D0)
#define T_InitPrint             ((void          __cdecl(*)())0x00439750)
#define T_Print                 ((TEXTSTRING    *__cdecl(*)(int16_t xpos, int16_t ypos, int16_t zpos, const char *string))0x00439780)
#define T_CentreH               ((void          __cdecl(*)(TEXTSTRING *text_string, int16_t enable))0x004399A0)
#define T_CentreV               ((void          __cdecl(*)(TEXTSTRING *text_string, int16_t enable))0x004399C0)
#define TempVideoAdjust         ((void          __cdecl(*)(int hires, double sizer))0x00416550)
#define ShowFatalError          ((void          __cdecl(*)(const char *message))0x0043D770)

void __cdecl init_game_malloc();
void __cdecl game_free(int free_size);
void __cdecl DB_Log(char *a1, ...);
const char* __cdecl GetFullPath(const char *filename);
int __cdecl FindCdDrive();
int __cdecl LoadRooms(FILE *fp);
void __cdecl LevelStats(int level_id);
int __cdecl S_LoadLevel(int level_id);
int __cdecl S_DrawHealthBar(int percent);
int __cdecl LoadItems(FILE *handle);
void __cdecl InitialiseLara();
void __cdecl InitialiseFXArray();
void __cdecl InitialiseLOTArray();

#endif
