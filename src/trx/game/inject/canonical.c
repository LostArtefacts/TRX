#include <trx/game/inject/canonical.h>

#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/vector.h>
#include <trx/debug.h>

// The transcoders parse TRXI's canonical records and rebuild them in the
// loaded game's native layout in memory, then hand the buffer to the level
// section readers - so those stay unaware the wire format is game-neutral.

typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} M_BUFFER;

static M_BUFFER M_BufferCreate(const size_t capacity)
{
    return (M_BUFFER) {
        .data = Memory_Alloc(capacity),
        .capacity = capacity,
    };
}

static void M_Put8(M_BUFFER *const buf, const uint8_t value)
{
    ASSERT(buf->size + 1 <= buf->capacity);
    buf->data[buf->size++] = (char)value;
}

static void M_Put16(M_BUFFER *const buf, const uint16_t value)
{
    M_Put8(buf, value & 0xFF);
    M_Put8(buf, value >> 8);
}

static void M_Put32(M_BUFFER *const buf, const uint32_t value)
{
    M_Put16(buf, value & 0xFFFF);
    M_Put16(buf, value >> 16);
}

static void M_PutData(
    M_BUFFER *const buf, TRX_FILE *const file, const size_t size)
{
    ASSERT(buf->size + size <= buf->capacity);
    File_ReadData(file, buf->data + buf->size, size);
    buf->size += size;
}

static void M_TranscodeMeshFaces(
    M_BUFFER *const buf, TRX_FILE *const fp, const int32_t vertex_count,
    const bool keep_effects, const bool keep_list)
{
    const uint16_t count = File_ReadU16(fp);
    if (keep_list) {
        M_Put16(buf, count);
    } else if (count > 0) {
        LOG_WARNING("dropping %d coloured faces for this game", count);
    }
    for (int32_t i = 0; i < count; i++) {
        for (int32_t j = 0; j < vertex_count + 1; j++) {
            const uint16_t value = File_ReadU16(fp);
            if (keep_list) {
                M_Put16(buf, value);
            }
        }
        const uint16_t effects = File_ReadU16(fp);
        if (keep_list && keep_effects) {
            M_Put16(buf, effects);
        }
    }
}

// Packs one rotation the way M_ParseMeshRotation unpacks it: the two-word
// form for every game, with TR1 storing the pair the other way round.
static void M_PackRotation(
    M_BUFFER *const buf, const int32_t game_version, const int16_t x,
    const int16_t y, const int16_t z)
{
    const uint16_t v1 =
        (((uint16_t)x >> 2) & 0x3FF0) | (((uint16_t)y >> 12) & 0xF);
    const uint16_t v2 =
        ((((uint16_t)y >> 6) & 0x3F) << 10) | (((uint16_t)z >> 6) & 0x3FF);
    if (game_version == 1) {
        M_Put16(buf, v2);
        M_Put16(buf, v1);
    } else {
        M_Put16(buf, v1);
        M_Put16(buf, v2);
    }
}

int32_t InjectCanonical_TranscodeObjectTextures(
    TRX_FILE *const src, const int32_t data_count, const int32_t game_version,
    char **const out_data)
{
    const bool tr4 = game_version == 4;
    M_BUFFER buf = M_BufferCreate((size_t)data_count * (tr4 ? 38 : 20));

    for (int32_t i = 0; i < data_count; i++) {
        M_PutData(&buf, src, 4); // attribute, tileAndFlag
        const uint16_t new_flags = File_ReadU16(src);
        if (tr4) {
            M_Put16(&buf, new_flags);
        }
        M_PutData(&buf, src, 16); // canonical uv bytes are the native u16s
        if (tr4) {
            M_PutData(&buf, src, 16); // originalU/V, width/height minus one
        } else {
            File_Skip(src, 16);
        }
    }

    *out_data = buf.data;
    return (int32_t)buf.size;
}

int32_t InjectCanonical_TranscodeAnimFrames(
    TRX_FILE *const src, const int32_t data_count, const int32_t game_version,
    char **const out_data, VECTOR *const out_offsets,
    VECTOR *const out_rot_counts)
{
    // Native frames never exceed the canonical ones: a canonical rotation is
    // six bytes against four packed, and the headers match.
    M_BUFFER buf = M_BufferCreate(
        (size_t)data_count * (9 + 1) * sizeof(int16_t)
        + (size_t)data_count * 123 * 4);

    for (int32_t i = 0; i < data_count; i++) {
        const int32_t offset = (int32_t)buf.size;
        Vector_Add(out_offsets, (void *)&offset);

        M_PutData(&buf, src, 18); // bounds and offset
        const uint16_t rot_count = File_ReadU16(src);
        const int32_t rots = rot_count;
        Vector_Add(out_rot_counts, (void *)&rots);
        if (game_version == 1) {
            M_Put16(&buf, rot_count);
        }
        for (int32_t r = 0; r < rot_count; r++) {
            const int16_t x = File_ReadS16(src);
            const int16_t y = File_ReadS16(src);
            const int16_t z = File_ReadS16(src);
            M_PackRotation(&buf, game_version, x, y, z);
        }
    }

    *out_data = buf.data;
    return (int32_t)buf.size;
}

int32_t InjectCanonical_TranscodeAnims(
    TRX_FILE *const src, const int32_t data_count, const int32_t game_version,
    const VECTOR *const frame_offsets, const VECTOR *const frame_rot_counts,
    char **const out_data)
{
    const bool tr4 = game_version == 4;
    M_BUFFER buf = M_BufferCreate((size_t)data_count * (tr4 ? 40 : 32));

    for (int32_t i = 0; i < data_count; i++) {
        const uint32_t ordinal = File_ReadU32(src);
        const uint8_t frame_rate = File_ReadU8(src);
        File_Skip(src, 1); // canonical frame size is derived below

        uint32_t frame_ofs = 0;
        uint8_t frame_size = 0;
        if (ordinal < (uint32_t)frame_offsets->count) {
            frame_ofs =
                *(const int32_t *)Vector_Get((VECTOR *)frame_offsets, ordinal);
            const int32_t rots = *(const int32_t *)Vector_Get(
                (VECTOR *)frame_rot_counts, ordinal);
            frame_size = (uint8_t)(9 + 2 * rots);
        } else if (frame_offsets->count > 0 || ordinal > 0) {
            LOG_WARNING("animation %d names no frame %u", i, ordinal);
        }

        M_Put32(&buf, frame_ofs);
        M_Put8(&buf, frame_rate);
        M_Put8(&buf, frame_size);
        M_PutData(&buf, src, 10); // stateID, speed, accel
        if (tr4) {
            M_PutData(&buf, src, 8); // lateral speed and accel
        } else {
            File_Skip(src, 8);
        }
        M_PutData(&buf, src, 16); // frameStart..animCommand
    }

    *out_data = buf.data;
    return (int32_t)buf.size;
}

int32_t InjectCanonical_TranscodeObjectMeshes(
    TRX_FILE *const src, const int32_t data_size, const int32_t game_version,
    int32_t *const offsets, const int32_t num_offsets, char **const out_data)
{
    const bool tr4 = game_version == 4;
    // Native meshes carry trailing padding the canonical records dropped, so
    // allow up to 4 extra bytes per mesh over the canonical size.
    M_BUFFER buf = M_BufferCreate(data_size + 4 * (data_size / 26 + 1));

    // Canonical mesh pointers are byte offsets into the canonical blob;
    // record where each mesh lands natively so they can be remapped.
    const size_t blob_start = File_Pos(src);
    VECTOR *const canonical_offsets = Vector_Create(sizeof(int32_t));
    VECTOR *const native_offsets = Vector_Create(sizeof(int32_t));

    while (File_Pos(src) < blob_start + data_size) {
        const int32_t canonical_pos = (int32_t)(File_Pos(src) - blob_start);
        const int32_t native_pos = (int32_t)buf.size;
        Vector_Add(canonical_offsets, (void *)&canonical_pos);
        Vector_Add(native_offsets, (void *)&native_pos);

        M_PutData(&buf, src, 6); // centre
        const int32_t radius = File_ReadS32(src);
        if (tr4) {
            M_Put32(&buf, (uint32_t)radius);
        } else {
            M_Put16(&buf, (uint16_t)(int16_t)radius);
            M_Put16(&buf, 0);
        }

        const int16_t vertex_count = File_ReadS16(src);
        M_Put16(&buf, (uint16_t)vertex_count);
        M_PutData(&buf, src, (size_t)vertex_count * 6);

        const int16_t normal_count = File_ReadS16(src);
        M_Put16(&buf, (uint16_t)normal_count);
        if (normal_count > 0) {
            M_PutData(&buf, src, (size_t)normal_count * 6);
        } else {
            M_PutData(&buf, src, (size_t)-normal_count * 2);
        }

        M_TranscodeMeshFaces(&buf, src, 4, tr4, true);
        M_TranscodeMeshFaces(&buf, src, 3, tr4, true);
        M_TranscodeMeshFaces(&buf, src, 4, false, !tr4);
        M_TranscodeMeshFaces(&buf, src, 3, false, !tr4);

        // The engine's mesh reader consumes two trailing bytes after the
        // face lists, and expects four-byte alignment for TR4.
        if (!tr4) {
            M_Put16(&buf, 0);
        } else {
            while (buf.size % 4 != 0) {
                M_Put8(&buf, 0);
            }
        }
    }

    for (int32_t i = 0; i < num_offsets; i++) {
        const int32_t index =
            Vector_IndexOf(canonical_offsets, (void *)&offsets[i]);
        if (index >= 0) {
            offsets[i] = *(const int32_t *)Vector_Get(native_offsets, index);
        } else {
            LOG_WARNING("mesh pointer %d sits on no mesh boundary", offsets[i]);
        }
    }
    Vector_Free(canonical_offsets);
    Vector_Free(native_offsets);

    *out_data = buf.data;
    return (int32_t)buf.size;
}
