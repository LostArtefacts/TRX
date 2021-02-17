#ifndef TR1MAIN_SPECIFIC_FRONTEND_H
#define TR1MAIN_SPECIFIC_FRONTEND_H

// clang-format off
#define S_FadeToBlack           ((void          __cdecl(*)())0x0041CD10)
#define S_DrawScreenBox         ((void          __cdecl(*)(int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, int32_t col, SG_COL* grdptr, uint16_t flags))0x0041C520)
#define S_DrawScreenFBox        ((void          __cdecl(*)(int32_t sx, int32_t sy, int32_t z, int32_t w, int32_t h, int32_t col, SG_COL* grdptr, uint16_t flags))0x0041CBB0)
// clang-format on

#endif
