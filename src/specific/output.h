#ifndef T1M_SPECIFIC_OUTPUT_H
#define T1M_SPECIFIC_OUTPUT_H

// clang-format off
#define S_AniamteTextures       ((void          (*)(int32_t nframes))0x00430660)
#define S_DumpScreen            ((int32_t       (*)())0x0042FC70)
#define S_ClearScreen           ((void          (*)())0x0042FCC0)
#define S_SetupAboveWater       ((void          (*)(int32_t underwater))0x00430640)
#define S_SetupBelowWater       ((void          (*)(int32_t underwater))0x004305E0)
#define S_InitialisePolyList    ((void          (*)())0x0042FC60)
#define S_OutputPolyList        ((void          (*)())0x0042FD10)
#define S_GetObjectBounds       ((int32_t       (*)(int16_t* bptr))0x0042FD30)
#define S_CalculateLight        ((void          (*)(int32_t x, int32_t y, int32_t z, int16_t room_num))0x00430100)
#define S_CalculateStaticLight  ((void          (*)(int16_t adder))0x00430290)
#define S_DisplayPicture        ((void          (*)(const char* filename))0x00430CE0)
#define S_DrawLightningSegment  ((void     (*)(int32_t x1, int32_t y1, int32_t z1, int32_t x2, int32_t y2, int32_t z2, int32_t width))0x00430740)
// clang-format on

void S_InitialiseScreen();
void S_DrawHealthBar(int32_t percent);
void S_DrawAirBar(int32_t percent);

int32_t GetRenderScaleGLRage(int32_t unit);
void RenderBar(int32_t value, int32_t value_max, int32_t bar_type);

int32_t GetRenderScale(int32_t unit);
int32_t GetRenderHeightDownscaled();
int32_t GetRenderWidthDownscaled();
int32_t GetRenderHeight();
int32_t GetRenderWidth();

void T1MInjectSpecificOutput();

#endif
