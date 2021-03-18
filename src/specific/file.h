#ifndef T1M_SPECIFIC_FILE_H
#define T1M_SPECIFIC_FILE_H

#include <stdint.h>
#include <stdio.h>

int32_t LoadLevel(const char *filename, int32_t level_num);
int32_t LoadRooms(FILE *fp);
int32_t LoadObjects(FILE *fp);
int32_t LoadSprites(FILE *fp);
int32_t LoadItems(FILE *fp);
int32_t LoadDepthQ(FILE *fp);
int32_t LoadPalette(FILE *fp);
int32_t LoadCameras(FILE *fp);
int32_t LoadSoundEffects(FILE *fp);
int32_t LoadBoxes(FILE *fp);
int32_t LoadAnimatedTextures(FILE *fp);
int32_t LoadCinematic(FILE *fp);
int32_t LoadDemo(FILE *fp);
int32_t LoadSamples(FILE *fp);
int32_t LoadTexturePages(FILE *fp);
int32_t S_LoadLevel(int32_t level_num);
const char *GetFullPath(const char *filename);

int32_t GetSecretCount();

void T1MInjectSpecificFile();

#endif
