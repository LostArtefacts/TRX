#ifndef T1M_GAME_SAVEGAME_H
#define T1M_GAME_SAVEGAME_H

#include <stdint.h>

// clang-format off
#define ExtractSaveGameInfo     ((void          (*)())0x00434F90)
// clang-format on

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

void CreateSaveGameInfo();

void ResetSG();
void WriteSG(void *pointer, int size);
void WriteSGARM(LARA_ARM *arm);
void WriteSGLara(LARA_INFO *lara);
void WriteSGLOT(LOT_INFO *lot);

void T1MInjectGameSaveGame();

#endif
