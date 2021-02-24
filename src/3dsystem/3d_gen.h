#ifndef T1M_3DSYSTEM_3D_GEN_H
#define T1M_3DSYSTEM_3D_GEN_H

// clang-format off
#define S_InsertRoom            ((void          (*)(int16_t* objptr))0x00401BD0)
#define phd_PushMatrix          ((void          (*)())0x0043EA01)
#define phd_PushUnitMatrix      ((void          (*)())0x0043EA21)
#define phd_TranslateAbs        ((void          (*)())0x004019A0)
#define phd_RotX                ((void          (*)(PHD_ANGLE angle))0x004012F0)
#define phd_RotY                ((void          (*)(PHD_ANGLE angle))0x004013A0)
#define phd_RotZ                ((void          (*)(PHD_ANGLE angle))0x00401450)
#define phd_RotYXZ              ((void          (*)(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz))0x00401500)
#define phd_RotYXZpack          ((void          (*)(int32_t rots))0x004016F0)
#define phd_PutPolygons         ((void          (*)(const int16_t* objptr, int clip))0x00401AD0)
#define phd_TranslateRel        ((int32_t       (*)(int32_t x, int32_t y, int32_t z))0x004018F0)
#define phd_GetVectorAngles     ((void          (*)(int32_t x, int32_t y, int32_t z, PHD_ANGLE* dest))0x00401270)
#define phd_GenerateW2V         ((void          (*)(PHD_3DPOS* viewpos))0x00401000)
#define phd_LookAt              ((void          (*)(int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar, int32_t ztar, PHD_ANGLE roll))0x004011A0)
// clang-format on

void phd_PopMatrix();

#endif
