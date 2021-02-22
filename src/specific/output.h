#ifndef TOMB1MAIN_SPECIFIC_OUTPUT_H
#define TOMB1MAIN_SPECIFIC_OUTPUT_H

// clang-format off
#define S_AniamteTextures       ((void          __cdecl(*)(int32_t nframes))0x00430660)
#define S_DumpScreen            ((int32_t       __cdecl(*)())0x0042FC70)
#define S_ClearScreen           ((void          __cdecl(*)())0x0042FCC0)
#define S_SetupAboveWater       ((void          __cdecl(*)(int underwater))0x00430640)
#define S_SetupBelowWater       ((void          __cdecl(*)(int underwater))0x004305E0)
#define S_InitialisePolyList    ((void          __cdecl(*)())0x0042FC60)
#define S_OutputPolyList        ((void          __cdecl(*)())0x0042FD10)
#define S_CopyBufferToScreen    ((void          __cdecl(*)())0x00416A60)
#define S_GetObjectBounds       ((int32_t       __cdecl(*)(int16_t* bptr))0x0042FD30)
#define S_CalculateLight        ((void          __cdecl(*)(int32_t x, int32_t y, int32_t z, int16_t room_num))0x00430100)
#define S_CalculateStaticLight  ((void          __cdecl(*)(int16_t adder))0x00430290)
// clang-format on

void __cdecl S_DrawHealthBar(int percent);
void __cdecl S_DrawAirBar(int percent);

typedef enum {
    BT_LARA_HEALTH = 0,
    BT_LARA_AIR = 1,
#if defined(TOMB1M_FEAT_UI) || defined(TOMB1M_FEAT_GAMEPLAY)
    BT_ENEMY_HEALTH = 2,
#endif
} BAR_TYPE;

int GetRenderScaleGLRage(int unit);
void RenderBar(int value, int value_max, int bar_type);

#ifdef TOMB1M_FEAT_UI
int GetRenderScale(int base);
int GetRenderHeightDownscaled();
int GetRenderWidthDownscaled();
int GetRenderHeight();
int GetRenderWidth();
#endif

void Tomb1MInjectSpecificOutput();

#endif
