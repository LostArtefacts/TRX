#ifndef T1M_GAME_SAVEGAME_H
#define T1M_GAME_SAVEGAME_H

#include <stdint.h>

void InitialiseStartInfo();
void ModifyStartInfo(int32_t level_num);
void CreateStartInfo(int level_num);

void CreateSaveGameInfo();
void ExtractSaveGameInfo();

void ResetSG();
void SkipSG(int size);
void ReadSG(void *pointer, int size);
void ReadSGARM(LARA_ARM *arm);
void ReadSGLara(LARA_INFO *lara);
void ReadSGLOT(LOT_INFO *lot);
void WriteSG(void *pointer, int size);
void WriteSGARM(LARA_ARM *arm);
void WriteSGLara(LARA_INFO *lara);
void WriteSGLOT(LOT_INFO *lot);

void T1MInjectGameSaveGame();

#endif
