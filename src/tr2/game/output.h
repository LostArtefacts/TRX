#pragma once

#include "global/types.h"

#include <stdint.h>

void __cdecl Output_Init(
    int16_t x, int16_t y, int32_t width, int32_t height, int32_t near_z,
    int32_t far_z, int16_t view_angle, int32_t screen_width,
    int32_t screen_height);

void __cdecl Output_InsertPolygons(const int16_t *obj_ptr, int32_t clip);
void __cdecl Output_InsertPolygons_I(const int16_t *ptr, int32_t clip);
void __cdecl Output_InsertRoom(const int16_t *obj_ptr, int32_t is_outside);
void __cdecl Output_InsertSkybox(const int16_t *obj_ptr);
const int16_t *__cdecl Output_CalcObjectVertices(const int16_t *obj_ptr);
const int16_t *__cdecl Output_CalcSkyboxLight(const int16_t *obj_ptr);
const int16_t *__cdecl Output_CalcVerticeLight(const int16_t *obj_ptr);
const int16_t *__cdecl Output_CalcRoomVertices(
    const int16_t *obj_ptr, int32_t far_clip);
void __cdecl Output_RotateLight(int16_t pitch, int16_t yaw);
void __cdecl Output_InitPolyList(void);
void __cdecl Output_SortPolyList(void);
void __cdecl Output_QuickSort(int32_t left, int32_t right);
void __cdecl Output_PrintPolyList(uint8_t *surface_ptr);
void __cdecl Output_SetNearZ(int32_t near_z);
void __cdecl Output_SetFarZ(int32_t far_z);
void __cdecl Output_AlterFOV(int16_t fov);

void __cdecl Output_DrawPolyLine(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyFlat(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyTrans(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyGouraud(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyGTMap(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyWGTMap(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyGTMapPersp(const int16_t *obj_ptr);
void __cdecl Output_DrawPolyWGTMapPersp(const int16_t *obj_ptr);

int32_t __cdecl Output_XGenX(const int16_t *obj_ptr);
int32_t __cdecl Output_XGenXG(const int16_t *obj_ptr);
int32_t __cdecl Output_XGenXGUV(const int16_t *obj_ptr);
int32_t __cdecl Output_XGenXGUVPerspFP(const int16_t *obj_ptr);
void __cdecl Output_GTMapPersp32FP(
    int32_t y1, int32_t y2, const uint8_t *tex_page);
void __cdecl Output_WGTMapPersp32FP(
    int32_t y1, int32_t y2, const uint8_t *tex_page);

int32_t __cdecl Output_VisibleZClip(
    const PHD_VBUF *vtx0, const PHD_VBUF *vtx1, const PHD_VBUF *vtx2);
int32_t __cdecl Output_ZedClipper(
    int32_t vtx_count, const POINT_INFO *pts, VERTEX_INFO *vtx);
int32_t __cdecl Output_XYClipper(int32_t vtx_count, VERTEX_INFO *vtx);
int32_t __cdecl Output_XYGClipper(int32_t vtx_count, VERTEX_INFO *vtx);
int32_t __cdecl Output_XYGUVClipper(int32_t vtx_count, VERTEX_INFO *vtx);

const int16_t *__cdecl Output_InsertObjectG3(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);
const int16_t *__cdecl Output_InsertObjectG4(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);
const int16_t *__cdecl Output_InsertObjectGT3(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);
const int16_t *__cdecl Output_InsertObjectGT4(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);

void __cdecl Output_InsertTrans8(const PHD_VBUF *vbuf, int16_t shade);
void __cdecl Output_InsertTransQuad(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t z);
void __cdecl Output_InsertFlatRect(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z,
    uint8_t color_idx);
void __cdecl Output_InsertLine(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z,
    uint8_t color_idx);

const int16_t *__cdecl Output_InsertObjectG3_ZBuffered(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);
const int16_t *__cdecl Output_InsertObjectG4_ZBuffered(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);
const int16_t *__cdecl Output_InsertObjectGT3_ZBuffered(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);
const int16_t *__cdecl Output_InsertObjectGT4_ZBuffered(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);

void __cdecl Output_InsertGT3_ZBuffered(
    const PHD_VBUF *vtx0, const PHD_VBUF *vtx1, const PHD_VBUF *vtx2,
    const PHD_TEXTURE *texture, const PHD_UV *uv0, const PHD_UV *uv1,
    const PHD_UV *uv2);

void __cdecl Output_InsertGT4_ZBuffered(
    const PHD_VBUF *vtx0, const PHD_VBUF *vtx1, const PHD_VBUF *vtx2,
    const PHD_VBUF *vtx3, const PHD_TEXTURE *texture);

void __cdecl Output_InsertFlatRect_ZBuffered(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z,
    uint8_t color_idx);

void __cdecl Output_InsertLine_ZBuffered(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z,
    uint8_t color_idx);

const int16_t *__cdecl Output_InsertObjectG3_Sorted(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);

const int16_t *__cdecl Output_InsertObjectG4_Sorted(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);

const int16_t *__cdecl Output_InsertObjectGT3_Sorted(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);

const int16_t *__cdecl Output_InsertObjectGT4_Sorted(
    const int16_t *obj_ptr, int32_t num, SORT_TYPE sort_type);

void __cdecl Output_InsertGT3_Sorted(
    const PHD_VBUF *vtx0, const PHD_VBUF *vtx1, const PHD_VBUF *vtx2,
    const PHD_TEXTURE *texture, const PHD_UV *uv0, const PHD_UV *uv1,
    const PHD_UV *uv2, SORT_TYPE sort_type);

void __cdecl Output_InsertGT4_Sorted(
    const PHD_VBUF *vtx0, const PHD_VBUF *vtx1, const PHD_VBUF *vtx2,
    const PHD_VBUF *vtx3, const PHD_TEXTURE *texture, SORT_TYPE sort_type);

void __cdecl Output_InsertFlatRect_Sorted(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z,
    uint8_t color_idx);

void __cdecl Output_InsertLine_Sorted(
    int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t z,
    uint8_t color_idx);

void __cdecl Output_InsertSprite_Sorted(
    int32_t z, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    int32_t sprite_idx, int16_t shade);

void __cdecl Output_InsertTrans8_Sorted(const PHD_VBUF *vbuf, int16_t shade);

void __cdecl Output_InsertTransQuad_Sorted(
    int32_t x, int32_t y, int32_t width, int32_t height, int32_t z);

void __cdecl Output_InsertSprite(
    int32_t z, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
    int32_t sprite_idx, int16_t shade);

const int16_t *__cdecl Output_InsertRoomSprite(
    const int16_t *obj_ptr, int32_t vtx_count);

void __cdecl Output_InsertClippedPoly_Textured(
    int32_t vtx_count, float z, int16_t poly_type, int16_t tex_page);

void __cdecl Output_InsertPoly_Gouraud(
    int32_t vtx_count, float z, int32_t red, int32_t green, int32_t blue,
    int16_t poly_type);

void __cdecl Output_DrawClippedPoly_Textured(int32_t vtx_count);

void __cdecl Output_DrawPoly_Gouraud(
    int32_t vtx_count, int32_t red, int32_t green, int32_t blue);

void __cdecl Output_DrawSprite(
    uint32_t flags, int32_t x, int32_t y, int32_t z, int16_t sprite_idx,
    int16_t shade, int16_t scale);

void __cdecl Output_DrawPickup(
    int32_t sx, int32_t sy, int32_t scale, int16_t sprite_idx, int16_t shade);

void __cdecl Output_DrawScreenSprite2D(
    int32_t sx, int32_t sy, int32_t sz, int32_t scale_h, int32_t scale_v,
    int16_t sprite_idx, int16_t shade, uint16_t flags);

void __cdecl Output_DrawScreenSprite(
    int32_t sx, int32_t sy, int32_t sz, int32_t scale_h, int32_t scale_v,
    int16_t sprite_idx, int16_t shade, uint16_t flags);

void __cdecl Output_DrawScaledSpriteC(const int16_t *obj_ptr);
