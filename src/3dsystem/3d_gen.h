#ifndef T1M_3DSYSTEM_3D_GEN_H
#define T1M_3DSYSTEM_3D_GEN_H

#include "global/types.h"

#include <stdint.h>

void phd_LookAt(
    int32_t xsrc, int32_t ysrc, int32_t zsrc, int32_t xtar, int32_t ytar,
    int32_t ztar, int16_t roll);
void phd_GetVectorAngles(int32_t x, int32_t y, int32_t z, int16_t *dest);
int32_t phd_VisibleZClip(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3);
void phd_RotateLight(int16_t pitch, int16_t yaw);
void phd_AlterFOV(PHD_ANGLE fov);

void phd_SetDrawDistFade(int32_t dist);
void phd_SetDrawDistMax(int32_t dist);
int32_t phd_GetDrawDistMin();
int32_t phd_GetDrawDistFade();
int32_t phd_GetDrawDistMax();
int32_t phd_GetNearZ();
int32_t phd_GetFarZ();

int32_t phd_CalculateFogShade(int32_t depth);

void phd_InitPolyList();
void phd_PutPolygons(const int16_t *obj_ptr, int clip);

void phd_PutPolygons_I(int16_t *ptr, int32_t clip);

void S_InsertRoom(const int16_t *obj_ptr);

const int16_t *calc_object_vertices(const int16_t *obj_ptr);
const int16_t *calc_vertice_light(const int16_t *obj_ptr);
const int16_t *calc_roomvert(const int16_t *obj_ptr);

#endif
