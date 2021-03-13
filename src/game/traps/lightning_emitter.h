#ifndef T1M_GAME_TRAPS_LIGHTNING_EMITTER_H
#define T1M_GAME_TRAPS_LIGHTNING_EMITTER_H

#include "game/types.h"

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

void SetupLightningEmitter(OBJECT_INFO *obj);
void InitialiseLightning(int16_t item_num);
void LightningControl(int16_t item_num);
void LightningCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll);
void DrawLightning(ITEM_INFO *item);

#endif
