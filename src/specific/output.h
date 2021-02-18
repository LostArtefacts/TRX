#ifndef TOMB1MAIN_SPECIFIC_OUTPUT_H
#define TOMB1MAIN_SPECIFIC_OUTPUT_H

// clang-format off
#define S_DumpScreen            ((void          __cdecl(*)())0x0042FC70)
#define S_SetupAboveWater       ((void          __cdecl(*)(int underwater))0x00430640)
#define S_SetupBelowWater       ((void          __cdecl(*)(int underwater))0x004305E0)
#define S_InitialisePolyList    ((void          __cdecl(*)())0x0042FC60)
#define S_OutputPolyList        ((void          __cdecl(*)())0x0042FD10)
#define S_CopyBufferToScreen    ((void          __cdecl(*)())0x00416A60)
#define S_GetObjectBounds       ((int32_t       __cdecl(*)(int16_t* bptr))0x0042FD30)
#define S_CalculateStaticLight  ((void          __cdecl(*)(int16_t adder))0x00430290)
// clang-format on

void __cdecl S_DrawHealthBar(int percent);
void __cdecl S_DrawAirBar(int percent);

void Tomb1MInjectSpecificOutput();

#endif
