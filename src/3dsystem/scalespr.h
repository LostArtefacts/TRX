#ifndef T1M_3DSYSTEM_SCALESPR_H
#define T1M_3DSYSTEM_SCALESPR_H

// clang-format off
#define S_DrawSprite            ((void         (*)(int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade))0x00435910)
#define S_DrawUISprite          ((void         (*)(int32_t x, int32_t y, int32_t scale, int16_t sprnum, int16_t brightness))0x00435D80)
#define S_DrawScreenSprite2d    ((void         (*)(int32_t sx, int32_t sy, int32_t z, int32_t scale_h, int32_t scale_v, int32_t sprnum, int16_t shade, uint16_t flags, int page))0x0041C180)
// clang-format on

#endif
