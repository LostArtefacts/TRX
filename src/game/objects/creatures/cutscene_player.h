#pragma once

#include "global/types.h"

#include <stdint.h>

void CutscenePlayer1_Setup(OBJECT_INFO *obj);
void CutscenePlayer2_Setup(OBJECT_INFO *obj);
void CutscenePlayer3_Setup(OBJECT_INFO *obj);
void CutscenePlayer4_Setup(OBJECT_INFO *obj);

void CutscenePlayer_Initialise(int16_t item_num);
void CutscenePlayer_Control(int16_t item_num);

void CutscenePlayer1_Initialise(int16_t item_num);
void CutscenePlayer4_Control(int16_t item_num);
