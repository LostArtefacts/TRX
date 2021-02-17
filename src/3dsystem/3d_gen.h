#ifndef TR1MAIN_3DSYSTEM_3D_GEN_H
#define TR1MAIN_3DSYSTEM_3D_GEN_H

// clang-format off
#define S_InsertRoom            ((void          __cdecl(*)(int16_t* objptr))0x00401BD0)
#define phd_PushMatrix          ((void          __cdecl(*)())0x0043EA01)
#define phd_TranslateAbs        ((void          __cdecl(*)())0x004019A0)
#define phd_RotX                ((void          __cdecl(*)(PHD_ANGLE angle))0x004012F0)
#define phd_RotY                ((void          __cdecl(*)(PHD_ANGLE angle))0x004013A0)
#define phd_RotZ                ((void          __cdecl(*)(PHD_ANGLE angle))0x00401450)
#define phd_PutPolygons         ((void          __cdecl(*)(const int16_t* objptr, int clip))0x00401AD0)
// clang-format on

void __cdecl phd_PopMatrix();

#endif
