#pragma once

#include "global/types.h"

#define LIGHTNING_DAMAGE 400
#define LIGHTNING_STEPS 8
#define LIGHTNING_RND ((64 << W2V_SHIFT) / 0x8000) // = 32
#define LIGHTNING_SHOOTS 2

typedef struct {
    int32_t onstate;
    int32_t count;
    int32_t zapped;
    int32_t notarget;
    PHD_VECTOR target;
    PHD_VECTOR main[LIGHTNING_STEPS], wibble[LIGHTNING_STEPS];
    int start[LIGHTNING_SHOOTS];
    PHD_VECTOR end[LIGHTNING_SHOOTS];
    PHD_VECTOR shoot[LIGHTNING_SHOOTS][LIGHTNING_STEPS];
} LIGHTNING;

void LightningEmitter_Setup(OBJECT_INFO *obj);
void LightningEmitter_Initialise(int16_t item_num);
void LightningEmitter_Control(int16_t item_num);
void LightningEmitter_Collision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void LightningEmitter_Draw(ITEM_INFO *item);
