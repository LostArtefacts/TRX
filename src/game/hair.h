#ifndef T1M_GAME_HAIR_H
#define T1M_GAME_HAIR_H

#include <stdint.h>

#ifdef T1M_FEAT_HAIR

    #define O_HAIR O_TEMP10

void InitialiseHair();
void HairControl(int32_t in_cutscene);
void DrawHair();
#endif

#endif
