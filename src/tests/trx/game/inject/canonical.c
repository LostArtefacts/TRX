#include <harness/harness.h>

#include <trx/core/file.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/game/inject/canonical.h>

// The TRXI canonical records have one layout for every game; these are the
// transcoders that rebuild them in a target game's native byte layout. Each
// test hand-writes canonical bytes and checks the native bytes per game, so
// a layout drifting on either side of the conversion fails here rather than
// in a level load.

static void M_Put16(char **const at, const uint16_t value)
{
    *(*at)++ = value & 0xFF;
    *(*at)++ = value >> 8;
}

static void M_Put32(char **const at, const uint32_t value)
{
    M_Put16(at, value & 0xFFFF);
    M_Put16(at, value >> 16);
}

static uint16_t M_Get16(const char *const data, const int32_t pos)
{
    return (uint8_t)data[pos] | ((uint8_t)data[pos + 1] << 8);
}

static uint32_t M_Get32(const char *const data, const int32_t pos)
{
    return M_Get16(data, pos) | ((uint32_t)M_Get16(data, pos + 2) << 16);
}

// One canonical object texture: attribute 1, tile 2, newFlags 3, four UVs
// 0x1111.., metadata 5..8.
static int32_t M_CanonicalTexture(char *const out)
{
    char *at = out;
    M_Put16(&at, 1);
    M_Put16(&at, 2);
    M_Put16(&at, 3);
    for (int32_t i = 0; i < 4; i++) {
        M_Put16(&at, 0x1111 * (i + 1)); // u
        M_Put16(&at, 0x2222 * (i + 1)); // v
    }
    M_Put32(&at, 5);
    M_Put32(&at, 6);
    M_Put32(&at, 7);
    M_Put32(&at, 8);
    return at - out;
}

TEST(object_textures_drop_the_tr4_extras_for_earlier_games)
{
    char canonical[38];
    CHECK_EQ_INT(M_CanonicalTexture(canonical), 38);
    TRX_FILE *const src = File_OpenBuffer(canonical, sizeof(canonical));

    char *native = nullptr;
    const int32_t size =
        InjectCanonical_TranscodeObjectTextures(src, 1, 1, &native);
    CHECK_EQ_INT(size, 20);
    CHECK_EQ_INT(M_Get16(native, 0), 1); // attribute
    CHECK_EQ_INT(M_Get16(native, 2), 2); // tile
    CHECK_EQ_INT(M_Get16(native, 4), 0x1111); // first u follows directly
    CHECK_EQ_INT(M_Get16(native, 18), 0x8888); // last v closes the record

    Memory_FreePointer(&native);
    File_Close(src);
}

TEST(object_textures_keep_every_field_for_tr4)
{
    char canonical[38];
    M_CanonicalTexture(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, sizeof(canonical));

    char *native = nullptr;
    const int32_t size =
        InjectCanonical_TranscodeObjectTextures(src, 1, 4, &native);
    CHECK_EQ_INT(size, 38);
    CHECK_EQ_INT(M_Get16(native, 4), 3); // newFlags kept
    CHECK_EQ_INT(M_Get16(native, 6), 0x1111); // uvs after it
    CHECK_EQ_INT((int32_t)M_Get32(native, 22), 5); // originalU
    CHECK_EQ_INT((int32_t)M_Get32(native, 34), 8); // heightMinusOne

    Memory_FreePointer(&native);
    File_Close(src);
}

// One canonical frame - bounds 1..6, offset 7..9, two mesh rotations with
// angles that survive the native packing (multiples of 64) - followed by a
// canonical animation naming it by ordinal.
static int32_t M_CanonicalFrame(char *const out)
{
    char *at = out;
    for (int32_t i = 1; i <= 9; i++) {
        M_Put16(&at, i); // bounds and offset
    }
    M_Put16(&at, 2); // rotation count
    M_Put16(&at, 0x1000); // rot 0 x
    M_Put16(&at, 0x2000);
    M_Put16(&at, 0x3000);
    M_Put16(&at, 64); // rot 1: the smallest angles the packing keeps
    M_Put16(&at, 128);
    M_Put16(&at, 192);
    return at - out;
}

TEST(frames_pack_the_rotations_for_tr2)
{
    char canonical[64];
    const int32_t size_in = M_CanonicalFrame(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, size_in);

    VECTOR *const offsets = Vector_Create(sizeof(int32_t));
    VECTOR *const rots = Vector_Create(sizeof(int32_t));
    char *native = nullptr;
    const int32_t size =
        InjectCanonical_TranscodeAnimFrames(src, 1, 2, &native, offsets, rots);

    // Header 18 bytes, then two two-word rotations - no count word.
    CHECK_EQ_INT(size, 18 + 2 * 4);
    CHECK_EQ_INT(M_Get16(native, 0), 1); // bounds arrive untouched
    CHECK_EQ_INT(M_Get16(native, 16), 9);
    // rot 0: x=0x1000 y=0x2000 z=0x3000 in the two-word form.
    CHECK_EQ_INT(
        M_Get16(native, 18), ((0x1000 >> 2) & 0x3FF0) | (0x2000 >> 12));
    CHECK_EQ_INT(
        M_Get16(native, 20), (((0x2000 >> 6) & 0x3F) << 10) | (0x3000 >> 6));
    CHECK_EQ_INT(offsets->count, 1);
    CHECK_EQ_INT(*(int32_t *)Vector_Get(offsets, 0), 0);
    CHECK_EQ_INT(*(int32_t *)Vector_Get(rots, 0), 2);

    Vector_Free(offsets);
    Vector_Free(rots);
    Memory_FreePointer(&native);
    File_Close(src);
}

TEST(frames_keep_the_count_word_and_swap_the_pair_for_tr1)
{
    char canonical[64];
    const int32_t size_in = M_CanonicalFrame(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, size_in);

    VECTOR *const offsets = Vector_Create(sizeof(int32_t));
    VECTOR *const rots = Vector_Create(sizeof(int32_t));
    char *native = nullptr;
    const int32_t size =
        InjectCanonical_TranscodeAnimFrames(src, 1, 1, &native, offsets, rots);

    // Header 18 bytes, the rotation count, then two two-word rotations.
    CHECK_EQ_INT(size, 18 + 2 + 2 * 4);
    CHECK_EQ_INT(M_Get16(native, 18), 2); // rotation count
    // TR1 stores the pair the other way round.
    CHECK_EQ_INT(
        M_Get16(native, 20), (((0x2000 >> 6) & 0x3F) << 10) | (0x3000 >> 6));
    CHECK_EQ_INT(
        M_Get16(native, 22), ((0x1000 >> 2) & 0x3FF0) | (0x2000 >> 12));

    Vector_Free(offsets);
    Vector_Free(rots);
    Memory_FreePointer(&native);
    File_Close(src);
}

// One canonical animation, 40 bytes, naming frame ordinal 0.
static void M_CanonicalAnim(char *const out)
{
    char *at = out;
    M_Put32(&at, 0); // frame ordinal
    *at++ = 2; // frameRate
    *at++ = 0; // frameSize, derived by the reader
    M_Put16(&at, 4); // stateID
    M_Put32(&at, 0x55555555); // speed
    M_Put32(&at, 0x66666666); // accel
    M_Put32(&at, 0x77777777); // lateralSpeed
    M_Put32(&at, 0x78787878); // lateralAccel
    for (int32_t i = 0; i < 8; i++) {
        M_Put16(&at, 0x100 + i); // frameStart..animCommand
    }
}

// Frame maps as the frames transcoder would leave them: one frame at native
// offset 36 with three rotations.
static void M_FrameMaps(VECTOR **offsets, VECTOR **rots)
{
    *offsets = Vector_Create(sizeof(int32_t));
    *rots = Vector_Create(sizeof(int32_t));
    const int32_t offset = 36;
    const int32_t rot_count = 3;
    Vector_Add(*offsets, (void *)&offset);
    Vector_Add(*rots, (void *)&rot_count);
}

TEST(anims_resolve_the_ordinal_and_drop_the_lateral_motion)
{
    char canonical[40];
    M_CanonicalAnim(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, sizeof(canonical));

    VECTOR *offsets, *rots;
    M_FrameMaps(&offsets, &rots);
    char *native = nullptr;
    const int32_t size =
        InjectCanonical_TranscodeAnims(src, 1, 2, offsets, rots, &native);
    CHECK_EQ_INT(size, 32);
    CHECK_EQ_INT((int32_t)M_Get32(native, 0), 36); // native byte offset
    CHECK_EQ_INT((uint8_t)native[5], 9 + 2 * 3); // derived frame size
    CHECK_EQ_INT((int32_t)M_Get32(native, 12), 0x66666666); // accel
    CHECK_EQ_INT(M_Get16(native, 16), 0x100); // frameStart follows accel

    Vector_Free(offsets);
    Vector_Free(rots);
    Memory_FreePointer(&native);
    File_Close(src);
}

TEST(anims_keep_the_lateral_motion_for_tr4)
{
    char canonical[40];
    M_CanonicalAnim(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, sizeof(canonical));

    VECTOR *offsets, *rots;
    M_FrameMaps(&offsets, &rots);
    char *native = nullptr;
    const int32_t size =
        InjectCanonical_TranscodeAnims(src, 1, 4, offsets, rots, &native);
    CHECK_EQ_INT(size, 40);
    CHECK_EQ_INT((int32_t)M_Get32(native, 16), 0x77777777); // lateralSpeed
    CHECK_EQ_INT(M_Get16(native, 24), 0x100); // frameStart after laterals

    Vector_Free(offsets);
    Vector_Free(rots);
    Memory_FreePointer(&native);
    File_Close(src);
}

// One canonical mesh: one vertex, one light, one textured triangle with
// effects, one coloured triangle - the families whose fate differs by game.
static int32_t M_CanonicalMesh(char *const out)
{
    char *at = out;
    M_Put16(&at, 10); // centre x
    M_Put16(&at, 11);
    M_Put16(&at, 12);
    M_Put32(&at, 600); // collRadius
    M_Put16(&at, 1); // vertexCount
    M_Put16(&at, 20);
    M_Put16(&at, 21);
    M_Put16(&at, 22);
    M_Put16(&at, (uint16_t)-1); // normalCount -1: one light follows
    M_Put16(&at, 0x1234); // light
    M_Put16(&at, 0); // textured quads
    M_Put16(&at, 1); // textured triangles
    M_Put16(&at, 0); // v0
    M_Put16(&at, 0);
    M_Put16(&at, 0);
    M_Put16(&at, 77); // texture
    M_Put16(&at, 5); // effects
    M_Put16(&at, 0); // coloured quads
    M_Put16(&at, 1); // coloured triangles
    M_Put16(&at, 0);
    M_Put16(&at, 0);
    M_Put16(&at, 0);
    M_Put16(&at, 88); // palette
    M_Put16(&at, 0); // effects
    return at - out;
}

TEST(meshes_narrow_the_radius_and_drop_effects_for_earlier_games)
{
    char canonical[64];
    const int32_t canonical_size = M_CanonicalMesh(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, canonical_size);

    int32_t offsets[] = { 0, 0 };
    char *native = nullptr;
    const int32_t size = InjectCanonical_TranscodeObjectMeshes(
        src, canonical_size, 1, offsets, 2, &native);

    CHECK_EQ_INT(M_Get16(native, 6), 600); // radius as s16
    CHECK_EQ_INT(M_Get16(native, 8), 0); // radius padding
    CHECK_EQ_INT(M_Get16(native, 18), (uint16_t)-1); // light count kept
    // Textured triangle: counts at 22 (quads=0) and 24 (tris=1), then three
    // vertices and the texture, with no effects word.
    CHECK_EQ_INT(M_Get16(native, 24), 1);
    CHECK_EQ_INT(M_Get16(native, 32), 77);
    // Coloured triangle survives for TR1 with its palette index.
    CHECK_EQ_INT(M_Get16(native, 34), 0); // coloured quads
    CHECK_EQ_INT(M_Get16(native, 36), 1); // coloured triangles
    CHECK_EQ_INT(M_Get16(native, 44), 88);
    CHECK_EQ_INT(size, 48); // two bytes of trailing padding
    // Both duplicate pointers remapped to the one mesh at native offset 0.
    CHECK_EQ_INT(offsets[0], 0);
    CHECK_EQ_INT(offsets[1], 0);

    Memory_FreePointer(&native);
    File_Close(src);
}

TEST(meshes_keep_effects_and_drop_coloured_faces_for_tr4)
{
    char canonical[64];
    const int32_t canonical_size = M_CanonicalMesh(canonical);
    TRX_FILE *const src = File_OpenBuffer(canonical, canonical_size);

    int32_t offsets[] = { 0 };
    char *native = nullptr;
    const int32_t size = InjectCanonical_TranscodeObjectMeshes(
        src, canonical_size, 4, offsets, 1, &native);

    CHECK_EQ_INT((int32_t)M_Get32(native, 6), 600); // radius as s32
    // Textured triangle keeps its effects word...
    CHECK_EQ_INT(M_Get16(native, 24), 1); // triangle count
    CHECK_EQ_INT(M_Get16(native, 32), 77); // texture
    CHECK_EQ_INT(M_Get16(native, 34), 5); // effects
    // ...and the coloured lists vanish entirely, counts included; the
    // record pads to a four-byte boundary.
    CHECK_EQ_INT(size, 36);

    Memory_FreePointer(&native);
    File_Close(src);
}

TEST(mesh_pointers_remap_onto_native_offsets)
{
    char canonical[128];
    int32_t first_size = M_CanonicalMesh(canonical);
    const int32_t second_size = M_CanonicalMesh(canonical + first_size);
    TRX_FILE *const src = File_OpenBuffer(canonical, first_size + second_size);

    int32_t offsets[] = { first_size, 0 };
    char *native = nullptr;
    InjectCanonical_TranscodeObjectMeshes(
        src, first_size + second_size, 1, offsets, 2, &native);

    // The second canonical mesh starts at the first native mesh's end,
    // including its trailing padding.
    CHECK_EQ_INT(offsets[0], 48);
    CHECK_EQ_INT(offsets[1], 0);

    Memory_FreePointer(&native);
    File_Close(src);
}
