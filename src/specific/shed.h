#ifndef T1M_SPECIFIC_SHED_H
#define T1M_SPECIFIC_SHED_H

// a place for odd functions that have no place to go yet

#include <stdint.h>

// clang-format off
#define sub_40837F              ((void          (*)())0x40837F)
#define sub_4380E0              ((void          (*)(int16_t *unk))0x004380E0)
#define S_DrawSpriteRel         ((void          (*)(int32_t x, int32_t y, int32_t z, int16_t sprnum, int16_t shade))0x00435B70)
// clang-format on

void SWRInit();

#endif
