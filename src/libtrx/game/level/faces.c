#include "game/level/common.h"
#include "game/objects.h"
#include "game/rooms.h"
#include "game/types.h"

#include <stdlib.h>

static int M_CompareFace4s(const void *a, const void *b);
static int M_CompareFace4s(const void *a, const void *b);
static int M_CompareRoomSprites(const void *a, const void *b);
static void M_SortRoomFaces(void);
static void M_SortObjectFaces(void);

static int M_CompareFace3s(const void *const a, const void *const b)
{
    const FACE3 *const face_a = a;
    const FACE3 *const face_b = b;
    return face_a->texture_idx - face_b->texture_idx;
}

static int M_CompareFace4s(const void *const a, const void *const b)
{
    const FACE4 *const face_a = a;
    const FACE4 *const face_b = b;
    return face_a->texture_idx - face_b->texture_idx;
}

static int M_CompareRoomSprites(const void *const a, const void *const b)
{
    const ROOM_SPRITE *const sprite_a = a;
    const ROOM_SPRITE *const sprite_b = b;
    return sprite_a->texture - sprite_b->texture;
}

static void M_SortRoomFaces(void)
{
    // sort room faces by material to reduce number of GPU flushes.
    for (int32_t i = 0; i < Room_GetCount(); i++) {
        ROOM *const room = Room_Get(i);
        qsort(
            room->mesh.face3s, room->mesh.num_face3s, sizeof(FACE3),
            M_CompareFace3s);
        qsort(
            room->mesh.face4s, room->mesh.num_face4s, sizeof(FACE4),
            M_CompareFace4s);
        qsort(
            room->mesh.sprites, room->mesh.num_sprites, sizeof(ROOM_SPRITE),
            M_CompareRoomSprites);
    }
}

static void M_SortObjectFaces(void)
{
    for (int32_t i = 0; i < Object_GetMeshCount(); i++) {
        const OBJECT_MESH *const mesh = Object_GetMesh(i);
        qsort(
            mesh->tex_face3s, mesh->num_tex_face3s, sizeof(FACE3),
            M_CompareFace3s);
        qsort(
            mesh->tex_face4s, mesh->num_tex_face4s, sizeof(FACE4),
            M_CompareFace4s);
    }
}

void Level_LoadFaces(void)
{
    M_SortRoomFaces();
    M_SortObjectFaces();
}
