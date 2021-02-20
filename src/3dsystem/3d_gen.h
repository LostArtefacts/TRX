#ifndef TOMB1MAIN_3DSYSTEM_3D_GEN_H
#define TOMB1MAIN_3DSYSTEM_3D_GEN_H

// clang-format off
#define S_InsertRoom            ((void          __cdecl(*)(int16_t* objptr))0x00401BD0)
#define phd_PushMatrix          ((void          __cdecl(*)())0x0043EA01)
#define phd_PushUnitMatrix      ((void          __cdecl(*)())0x0043EA21)
#define phd_TranslateAbs        ((void          __cdecl(*)())0x004019A0)
#define phd_RotX                ((void          __cdecl(*)(PHD_ANGLE angle))0x004012F0)
#define phd_RotY                ((void          __cdecl(*)(PHD_ANGLE angle))0x004013A0)
#define phd_RotZ                ((void          __cdecl(*)(PHD_ANGLE angle))0x00401450)
#define phd_RotYXZ              ((void          __cdecl(*)(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz))0x00401500)
#define phd_RotYXZpack          ((void          __cdecl(*)(int32_t rots))0x004016F0)
#define phd_PutPolygons         ((void          __cdecl(*)(const int16_t* objptr, int clip))0x00401AD0)
#define phd_TranslateRel        ((int32_t       __cdecl(*)(int32_t x, int32_t y, int32_t z))0x004018F0)
#define phd_GetVectorAngles     ((void          __cdecl(*)(int32_t x, int32_t y, int32_t z, PHD_ANGLE* dest))0x00401270)
#define phd_GenerateW2V         ((void          __cdecl(*)(PHD_3DPOS* viewpos))0x00401000)
// clang-format on

void __cdecl phd_PopMatrix();

#endif
