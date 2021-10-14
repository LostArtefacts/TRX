#ifndef T1M_3DSYSTEM_3D_GEN_H
#define T1M_3DSYSTEM_3D_GEN_H

#include "global/types.h"

#include <stdint.h>

// clang-format off
#define S_InsertRoom            ((void          (*)(int16_t* objptr))0x00401BD0)
#define phd_PutPolygons         ((void          (*)(const int16_t* objptr, int clip))0x00401AD0)
#define phd_InitPolyList        ((void          (*)())0x00402470)
// clang-format on

void phd_GenerateW2V(PHD_3DPOS *viewpos);
void phd_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll);
void phd_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest);
void phd_RotX(PHD_ANGLE rx);
void phd_RotY(PHD_ANGLE ry);
void phd_RotZ(PHD_ANGLE rz);
void phd_RotYXZ(PHD_ANGLE ry, PHD_ANGLE rx, PHD_ANGLE rz);
void phd_RotYXZpack(int32_t rots);
int32_t phd_TranslateRel(int32_t x, int32_t y, int32_t z);
void phd_TranslateAbs(int32_t x, int32_t y, int32_t z);
int32_t visible_zclip(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3);
void phd_RotateLight(int16_t pitch, int16_t yaw);
void phd_InitWindow(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t nearz,
    int32_t farz, int32_t view_angle, int32_t scrwidth, int32_t scrheight,
    uint8_t *scrptr);
void AlterFOV(PHD_ANGLE fov);

void phd_PushMatrix();
void phd_PushUnitMatrix();
void phd_PopMatrix();

int16_t *calc_object_vertices(int16_t *obj_ptr);
int16_t *calc_vertice_light(int16_t *obj_ptr);
int16_t *calc_roomvert(int16_t *obj_ptr);

void T1MInject3DSystem3DGen();

#endif
