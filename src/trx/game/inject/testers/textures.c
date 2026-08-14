#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/game/inject.h>
#include <trx/game/output.h>

static bool M_TestTextureSample(
    const INJECTION_CONTEXT *const ctx, const INJECTION *const injection)
{
    const int32_t texture_idx = File_ReadS32(injection->fp);
    OBJECT_TEXTURE test_texture = {};
    test_texture.draw_type = File_ReadU16(injection->fp);
    test_texture.tex_page = File_ReadU16(injection->fp);
    for (int32_t i = 0; i < 4; i++) {
        test_texture.uv[i].u = File_ReadU16(injection->fp);
        test_texture.uv[i].v = File_ReadU16(injection->fp);
    }

    if (texture_idx < 0 || texture_idx >= Output_GetObjectTextureCount()) {
        return false;
    }

    const OBJECT_TEXTURE *const texture = Output_GetObjectTexture(texture_idx);

    if (test_texture.draw_type != texture->draw_type
        || test_texture.tex_page != texture->tex_page) {
        return false;
    }

    for (int32_t i = 0; i < 4; i++) {
        if (test_texture.uv[i].u != texture->uv[i].u
            || test_texture.uv[i].v != texture->uv[i].v) {
            return false;
        }
    }

    return true;
}

REGISTER_INJECT_TESTER(ITT_TEXTURE_SAMPLE, M_TestTextureSample)
