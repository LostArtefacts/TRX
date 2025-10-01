#include "game/level/common.h"
#include "game/objects.h"
#include "game/output.h"
#include "game/rooms.h"
#include "game/types.h"

#include <stdlib.h>

static int M_CompareFace3s(const void *const a, const void *const b)
{
    const FACE3 *const face_a = a;
    const FACE3 *const face_b = b;
    const OBJECT_TEXTURE *const tex_a =
        Output_GetObjectTexture(face_a->texture_idx);
    const OBJECT_TEXTURE *const tex_b =
        Output_GetObjectTexture(face_b->texture_idx);
    return tex_a->tex_page - tex_b->tex_page;
}

static int M_CompareFace4s(const void *const a, const void *const b)
{
    const FACE4 *const face_a = a;
    const FACE4 *const face_b = b;
    const OBJECT_TEXTURE *const tex_a =
        Output_GetObjectTexture(face_a->texture_idx);
    const OBJECT_TEXTURE *const tex_b =
        Output_GetObjectTexture(face_b->texture_idx);
    return tex_a->tex_page - tex_b->tex_page;
}

static int M_CompareRoomSprites(const void *const a, const void *const b)
{
    const ROOM_SPRITE *const sprite_a = a;
    const ROOM_SPRITE *const sprite_b = b;
    const OBJECT_TEXTURE *const tex_a =
        Output_GetObjectTexture(sprite_a->texture);
    const OBJECT_TEXTURE *const tex_b =
        Output_GetObjectTexture(sprite_b->texture);
    return tex_a->tex_page - tex_b->tex_page;
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
