#ifndef T1M_GAME_SAVEGAME_H
#define T1M_GAME_SAVEGAME_H

#include <stdint.h>

// clang-format off
#define CreateStartInfo         ((void          (*)(int32_t level_num))0x004345E0)
#define CreateSaveGameInfo      ((void          (*)())0x00434720)
#define ExtractSaveGameInfo     ((void          (*)())0x00434F90)
// clang-format on

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);

void T1MInjectGameSaveGame();

#endif
