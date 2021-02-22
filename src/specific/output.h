#ifndef T1M_SPECIFIC_OUTPUT_H
#define T1M_SPECIFIC_OUTPUT_H

// clang-format off
#define S_AniamteTextures       ((void         (*)(int32_t nframes))0x00430660)
#define S_DumpScreen            ((int32_t      (*)())0x0042FC70)
#define S_ClearScreen           ((void         (*)())0x0042FCC0)
#define S_SetupAboveWater       ((void         (*)(int underwater))0x00430640)
#define S_SetupBelowWater       ((void         (*)(int underwater))0x004305E0)
#define S_InitialisePolyList    ((void         (*)())0x0042FC60)
#define S_OutputPolyList        ((void         (*)())0x0042FD10)
#define S_CopyBufferToScreen    ((void         (*)())0x00416A60)
#define S_GetObjectBounds       ((int32_t      (*)(int16_t* bptr))0x0042FD30)
#define S_CalculateLight        ((void         (*)(int32_t x, int32_t y, int32_t z, int16_t room_num))0x00430100)
#define S_CalculateStaticLight  ((void         (*)(int16_t adder))0x00430290)
// clang-format on

void S_DrawHealthBar(int percent);
void S_DrawAirBar(int percent);

typedef enum {
    BT_LARA_HEALTH = 0,
    BT_LARA_AIR = 1,
#if defined(T1M_FEAT_UI) || defined(TOMB1M_FEAT_GAMEPLAY)
    BT_ENEMY_HEALTH = 2,
#endif
} BAR_TYPE;

int GetRenderScaleGLRage(int unit);
void RenderBar(int value, int value_max, int bar_type);

#ifdef T1M_FEAT_UI
int GetRenderScale(int base);
int GetRenderHeightDownscaled();
int GetRenderWidthDownscaled();
int GetRenderHeight();
int GetRenderWidth();
#endif

void T1MInjectSpecificOutput();

#endif
