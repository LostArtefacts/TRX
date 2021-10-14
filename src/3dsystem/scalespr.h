#ifndef T1M_3DSYSTEM_SCALESPR_H
#define T1M_3DSYSTEM_SCALESPR_H

#include <stdint.h>

// clang-format off
// clang-format on

void S_DrawSprite(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void S_DrawSpriteRel(
    int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade);
void S_DrawUISprite(
    int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t shade);

void T1MInject3DSystemScaleSpr();

#endif
