// The injection container, far enough to reach the font's sprite textures.
//
// Reading the shipped font.bin rather than a table generated alongside it means
// the tests measure text with the same metrics the game does, and that there is
// nothing to keep in step when the font changes.
//
// Layout: the "TRXJ" header, then a zlib payload holding the level tests, then
// the chunks. Each chunk is a type, a block count and a total size; each block
// a data type, an item count and a payload size. The sprite textures and the
// sprite sequences naming the fonts both live in the texture chunks.

#include <harness/font_bin.h>

#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/game/inject/enum.h>

#include <stdio.h>
#include <string.h>
#include <zlib.h>

#define M_MAGIC 0x4A585254 // "TRXJ"
#define M_SPRITE_RECORD_SIZE 16

typedef struct {
    const char *data;
    size_t size;
    size_t pos;
} M_CURSOR;

static bool M_CanRead(const M_CURSOR *const cursor, const size_t bytes)
{
    return cursor->pos + bytes <= cursor->size;
}

static int32_t M_ReadS32(M_CURSOR *const cursor)
{
    int32_t value = 0;
    memcpy(&value, cursor->data + cursor->pos, sizeof(value));
    cursor->pos += sizeof(value);
    return value;
}

static int16_t M_ReadS16(M_CURSOR *const cursor)
{
    int16_t value = 0;
    memcpy(&value, cursor->data + cursor->pos, sizeof(value));
    cursor->pos += sizeof(value);
    return value;
}

static uint16_t M_ReadU16(M_CURSOR *const cursor)
{
    uint16_t value = 0;
    memcpy(&value, cursor->data + cursor->pos, sizeof(value));
    cursor->pos += sizeof(value);
    return value;
}

static char *M_LoadPayload(const char *const path, size_t *const out_size)
{
    FILE *const fp = fopen(path, "rb");
    if (fp == nullptr) {
        return nullptr;
    }
    fseek(fp, 0, SEEK_END);
    const long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *const raw = Memory_Alloc(file_size);
    const size_t read_size = fread(raw, 1, file_size, fp);
    fclose(fp);

    char *payload = nullptr;
    M_CURSOR header = { .data = raw, .size = read_size };
    if (read_size < 20 || (uint32_t)M_ReadS32(&header) != M_MAGIC) {
        goto cleanup;
    }
    M_ReadS32(&header); // version
    M_ReadS32(&header); // injection type
    const int32_t uncompressed_size = M_ReadS32(&header);
    const int32_t compressed_size = M_ReadS32(&header);
    if (uncompressed_size <= 0
        || header.pos + (size_t)compressed_size > read_size) {
        goto cleanup;
    }

    payload = Memory_Alloc(uncompressed_size);
    uLongf actual_size = uncompressed_size;
    if (uncompress(
            (Bytef *)payload, &actual_size, (const Bytef *)raw + header.pos,
            compressed_size)
        != Z_OK) {
        Memory_FreePointer(&payload);
        goto cleanup;
    }
    *out_size = actual_size;

cleanup:
    Memory_Free(raw);
    return payload;
}

static void M_ReadSprites(
    M_CURSOR *const cursor, const int32_t count, FONT_BIN *const out)
{
    if (!M_CanRead(cursor, (size_t)count * M_SPRITE_RECORD_SIZE)) {
        return;
    }
    out->sprites = Memory_Realloc(
        out->sprites, sizeof(SPRITE_TEXTURE) * (out->sprite_count + count));
    for (int32_t i = 0; i < count; i++) {
        SPRITE_TEXTURE *const sprite = &out->sprites[out->sprite_count + i];
        *sprite = (SPRITE_TEXTURE) {
            .tex_page = M_ReadU16(cursor),
            .offset = M_ReadU16(cursor),
            .width = M_ReadU16(cursor),
            .height = M_ReadU16(cursor),
            .x0 = M_ReadS16(cursor),
            .y0 = M_ReadS16(cursor),
            .x1 = M_ReadS16(cursor),
            .y1 = M_ReadS16(cursor),
        };
    }
    out->sprite_count += count;
}

// A sequence's mesh index counts from the sprites the injection has appended so
// far, the way the game accumulates it while applying them.
static void M_ReadSequences(
    M_CURSOR *const cursor, const int32_t count, FONT_BIN *const out)
{
    int32_t base = 0;
    int32_t font_idx = 0;
    for (int32_t i = 0; i < count && M_CanRead(cursor, 12); i++) {
        M_ReadS32(cursor); // object type
        M_ReadS32(cursor); // object id
        // A sprite sequence stores its count negated. Read it out before taking
        // the absolute value: ABS is a macro, and would consume the field
        // twice.
        const int16_t raw_count = M_ReadS16(cursor);
        const int32_t mesh_count = ABS(raw_count);
        const int32_t mesh_idx = M_ReadS16(cursor);
        if (font_idx < FONT_BIN_FONT_COUNT) {
            out->font_base[font_idx] = base + mesh_idx;
            out->font_count[font_idx] = mesh_count;
            font_idx++;
        }
        base += mesh_count;
    }
}

bool FontBin_Load(const char *const path, FONT_BIN *const out)
{
    *out = (FONT_BIN) {};

    size_t payload_size = 0;
    char *const payload = M_LoadPayload(path, &payload_size);
    if (payload == nullptr) {
        return false;
    }

    M_CURSOR cursor = { .data = payload, .size = payload_size };
    if (!M_CanRead(&cursor, 12)) {
        Memory_Free(payload);
        return false;
    }
    M_ReadS32(&cursor); // level test count
    cursor.pos += M_ReadS32(&cursor); // level test payload
    const int32_t chunk_count = M_ReadS32(&cursor);

    for (int32_t i = 0; i < chunk_count && M_CanRead(&cursor, 12); i++) {
        const int32_t chunk_type = M_ReadS32(&cursor);
        const int32_t block_count = M_ReadS32(&cursor);
        const int32_t chunk_size = M_ReadS32(&cursor);
        if (chunk_type != ICT_TEXTURE_DATA && chunk_type != ICT_TEXTURE_INFO) {
            cursor.pos += chunk_size;
            continue;
        }

        for (int32_t j = 0; j < block_count && M_CanRead(&cursor, 12); j++) {
            const int32_t data_type = M_ReadS32(&cursor);
            const int32_t data_count = M_ReadS32(&cursor);
            const int32_t data_size = M_ReadS32(&cursor);
            const size_t block_end = cursor.pos + data_size;
            if (data_type == IDT_SPRITE_TEXTURES) {
                M_ReadSprites(&cursor, data_count, out);
            } else if (data_type == IDT_SPRITE_SEQUENCES) {
                M_ReadSequences(&cursor, data_count, out);
            }
            cursor.pos = block_end;
        }
    }

    Memory_Free(payload);
    return out->sprite_count > 0;
}

void FontBin_Free(FONT_BIN *const font_bin)
{
    Memory_FreePointer(&font_bin->sprites);
    *font_bin = (FONT_BIN) {};
}
