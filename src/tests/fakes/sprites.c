#include <fakes/sprites.h>

#include <trx/game/objects/common.h>
#include <trx/game/output/textures.h>

#define M_MAX_SPRITES 64

static SPRITE_TEXTURE m_Sprites[M_MAX_SPRITES];
static int32_t m_SpriteCount = 0;

void FakeSprites_Define(
    const OBJECT_ID object_id, const int32_t sprite_count, const int32_t width,
    const int32_t height)
{
    const int32_t base = m_SpriteCount;
    for (int32_t i = 0; i < sprite_count && m_SpriteCount < M_MAX_SPRITES;
         i++) {
        m_Sprites[m_SpriteCount++] = (SPRITE_TEXTURE) {
            .x0 = -width / 2,
            .y0 = -height,
            .x1 = width - width / 2,
            .y1 = 0,
        };
    }
    *Object_Get(object_id) = (OBJECT) {
        .loaded = true,
        .mesh_idx = base,
        .mesh_count = -(m_SpriteCount - base),
    };
}

void FakeSprites_Forget(void)
{
    m_SpriteCount = 0;
}

SPRITE_TEXTURE *Output_GetSpriteTexture(const int32_t texture_idx)
{
    if (texture_idx < 0 || texture_idx >= m_SpriteCount) {
        return nullptr;
    }
    return &m_Sprites[texture_idx];
}
