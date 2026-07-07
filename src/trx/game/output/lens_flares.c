#include <trx/game/output/lens_flares.h>

#include <trx/core/math.h>
#include <trx/core/math/const.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/collision/los.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects/common.h>
#include <trx/game/output/shaders/mesh.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/state.h>
#include <trx/game/output/vars.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks/enum.h>
#include <trx/game/viewport.h>

#include <math.h>

// The flare works in the OG 640-wide virtual screen space: the halo is active
// while the flare sits within this distance of the screen center, and the
// proximity doubles as the halo intensity.
#define M_MAX_CENTER_DIST 640
// Proximity beyond which the full-screen flash kicks in.
#define M_FLASH_THRESHOLD 544
#define M_FLASH_FADER_MAX 32
// View depth (world units) the screen-space ghost trail is anchored at; the
// negative clip-space depth bias then pulls it in front of the scene like the
// OG's near-plane sprites. The depth-tested sun disk uses a separate,
// far-plane-relative distance instead (see M_GetSunDiskViewDist) so that
// scene geometry anywhere in the view volume can occlude it.
#define M_GHOST_VIEW_DIST 8192

typedef struct {
    RGB_888 color;
    uint8_t size;
    // Position along the flare-to-screen-center line, in 16ths of the
    // distance; negative overshoots past the flare.
    int8_t line_pos;
    // Slot in the combined O_SPARKS_GFX atlas. The OG flare_table indexes
    // TR4's DEFAULT_SPRITES; those numbers are baked here to their SPARK_TYPE
    // equivalents (see game/sparks/enum.h).
    uint8_t sprite;
} M_GHOST;

// OG flare_table; the first entry is the sun glare drawn at the flare itself,
// which placeable object flares skip.
static const M_GHOST m_Ghosts[] = {
    // clang-format off
    { .color = { 96, 80, 0 },    .size = 6,  .line_pos = 0,  .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 48, 32, 32 },   .size = 10, .line_pos = -6, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 32, 24, 24 },   .size = 18, .line_pos = -1, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 80, 104, 64 },  .size = 5,  .line_pos = -3, .sprite = SPARK_TYPE_LENS_FLARE_3 },
    { .color = { 64, 64, 64 },   .size = 20, .line_pos = 0,  .sprite = SPARK_TYPE_LENS_FLARE_5 },
    { .color = { 96, 56, 56 },   .size = 14, .line_pos = 0,  .sprite = SPARK_TYPE_LENS_FLARE_1 },
    { .color = { 80, 40, 32 },   .size = 9,  .line_pos = 0,  .sprite = SPARK_TYPE_LENS_FLARE_2 },
    { .color = { 16, 24, 40 },   .size = 2,  .line_pos = 5,  .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 8, 8, 24 },     .size = 7,  .line_pos = 8,  .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 8, 16, 32 },    .size = 4,  .line_pos = 10, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 48, 24, 0 },    .size = 2,  .line_pos = 13, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 40, 96, 72 },   .size = 1,  .line_pos = 16, .sprite = SPARK_TYPE_LENS_FLARE_1 },
    { .color = { 40, 96, 72 },   .size = 3,  .line_pos = 20, .sprite = SPARK_TYPE_LENS_FLARE_1 },
    { .color = { 32, 16, 0 },    .size = 6,  .line_pos = 22, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 32, 16, 0 },    .size = 9,  .line_pos = 23, .sprite = SPARK_TYPE_LENS_FLARE_3 },
    { .color = { 32, 16, 0 },    .size = 3,  .line_pos = 24, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 32, 48, 24 },   .size = 4,  .line_pos = 26, .sprite = SPARK_TYPE_LENS_FLARE_4 },
    { .color = { 8, 40, 112 },   .size = 3,  .line_pos = 27, .sprite = SPARK_TYPE_LENS_FLARE_1 },
    { .color = { 8, 16, 0 },     .size = 10, .line_pos = 29, .sprite = SPARK_TYPE_LENS_FLARE_3 },
    { .color = { 16, 16, 24 },   .size = 17, .line_pos = 31, .sprite = SPARK_TYPE_LENS_FLARE_2 },
    // clang-format on
};

static bool m_SunEnabled = false;
static XYZ_32 m_SunPos = {};
static RGB_888 m_SunColor = {};
static int32_t m_FlashFader = 0;
static RGB_888 m_FlashColor = {};

static int32_t M_GetSpriteIndex(const int32_t slot)
{
    const OBJECT *const obj = Object_Get(O_SPARKS_GFX);
    if (obj == nullptr || !obj->loaded || slot >= ABS(obj->mesh_count)) {
        return -1;
    }
    return obj->mesh_idx + slot;
}

// Projects a world point with the current view matrix into the 640-wide
// virtual screen space. Returns false if the point is behind the camera.
static bool M_ProjectToScreen(
    const XYZ_32 pos, float *const out_x, float *const out_y)
{
    const MATRIX *const m = &g_ViewMatrix;
    const double w2v = (double)(1 << W2V_SHIFT);
    // Rotate the camera-relative vector rather than using the matrix
    // translation column: with absolute world coordinates the int32 products
    // can overflow.
    const XYZ_32 rel = {
        .x = pos.x - g_ViewPos.x,
        .y = pos.y - g_ViewPos.y,
        .z = pos.z - g_ViewPos.z,
    };
    const double zv = ((double)m->_20 * rel.x + (double)m->_21 * rel.y
                       + (double)m->_22 * rel.z)
        / w2v;
    if (zv <= 0.0) {
        return false;
    }
    const double xv = ((double)m->_00 * rel.x + (double)m->_01 * rel.y
                       + (double)m->_02 * rel.z)
        / w2v;
    const double yv = ((double)m->_10 * rel.x + (double)m->_11 * rel.y
                       + (double)m->_12 * rel.z)
        / w2v;
    const double scale = 640.0 / Viewport_GetWidth(VIEWPORT_GAME);
    *out_x = (float)((Viewport_GetCenterX(VIEWPORT_GAME) + xv * g_PhdPersp / zv)
                     * scale);
    *out_y = (float)((Viewport_GetCenterY(VIEWPORT_GAME) + yv * g_PhdPersp / zv)
                     * scale);
    return true;
}

// View depth (world units) to anchor the depth-tested sun disk at. Staying
// fixed at M_GHOST_VIEW_DIST would place the disk closer to the camera than
// the far clip plane (Output_GetFarZ(), typically tens of thousands of
// units), so any occluder beyond M_GHOST_VIEW_DIST - a distant wall, a
// mountain across an outdoor area - would fail the depth test and the disk
// would flash through it. Anchoring just inside the far plane instead lets
// scene geometry anywhere in the view volume occlude it correctly.
static double M_GetSunDiskViewDist(void)
{
    return (double)Output_GetFarZ() / (double)(1 << W2V_SHIFT) * 0.98;
}

// Stages a screen-space additive billboard: the anchor is the given virtual
// screen position unprojected to view_dist. When depth_test is false a
// clip-space bias keeps it in front of the scene (the ghost trail, like the
// OG's near-plane sprites); when true the billboard keeps its true depth so
// scene geometry occludes it (the sun disk, drawn near the far plane).
static void M_StageGhost(
    const float virt_x, const float virt_y, const float virt_half_size,
    const RGBA_8888 color, const int32_t sprite_idx, const bool depth_test,
    const double view_dist)
{
    const double inv_scale = Viewport_GetWidth(VIEWPORT_GAME) / 640.0;
    const double off_x =
        virt_x * inv_scale - Viewport_GetCenterX(VIEWPORT_GAME);
    const double off_y =
        virt_y * inv_scale - Viewport_GetCenterY(VIEWPORT_GAME);
    const double fx = off_x * view_dist / g_PhdPersp;
    const double fy = off_y * view_dist / g_PhdPersp;

    const MATRIX *const m = &g_ViewMatrix;
    const double w2v = (double)(1 << W2V_SHIFT);
    const XYZ_32 anchor = {
        .x = g_ViewPos.x
            + (int32_t)((m->_00 * fx + m->_10 * fy + m->_20 * view_dist) / w2v),
        .y = g_ViewPos.y
            + (int32_t)((m->_01 * fx + m->_11 * fy + m->_21 * view_dist) / w2v),
        .z = g_ViewPos.z
            + (int32_t)((m->_02 * fx + m->_12 * fy + m->_22 * view_dist) / w2v),
    };

    const float h =
        (float)(virt_half_size * inv_scale * view_dist / g_PhdPersp);
    const XYZ_32 world_pos[4] = { anchor, anchor, anchor, anchor };
    const float disp[4][2] = {
        { -h, -h },
        { -h, h },
        { h, h },
        { h, -h },
    };
    const RGBA_8888 colors[4] = { color, color, color, color };
    OutputSource_PolyFX_StageQuadExtDepth(
        sprite_idx, world_pos, disp, colors,
        VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_BILLBOARD | VERT_ABS_SPRITE,
        depth_test ? 0.0f : -(float)view_dist, DRAW_BLEND_ADD);
}

static void M_StageFlare(
    const XYZ_32 flare_pos, const int16_t flare_room, const RGB_888 color,
    const bool is_object)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }

    XYZ_32 pos = flare_pos;
    int16_t room_num = NO_ROOM;

    if (is_object) {
        if (ABS(pos.x - lara_item->pos.x) > 0x8000
            || ABS(pos.y - lara_item->pos.y) > 0x8000
            || ABS(pos.z - lara_item->pos.z) > 0x8000) {
            return;
        }
        room_num = flare_room;
    } else {
        if (Room_Get(g_Camera.pos.room_num)->flags.no_lens_flare) {
            return;
        }

        // The sun sits at a far-away direction point; step it toward the
        // camera until it is close enough to resolve a room for it.
        while (ABS(pos.x) > 0x36000 || ABS(pos.y) > 0x36000
               || ABS(pos.z) > 0x36000) {
            pos.x -= (flare_pos.x - g_Camera.pos.x) >> 4;
            pos.y -= (flare_pos.y - g_Camera.pos.y) >> 4;
            pos.z -= (flare_pos.z - g_Camera.pos.z) >> 4;
        }

        XYZ_32 step = {
            .x = (pos.x - g_Camera.pos.x) >> 4,
            .y = (pos.y - g_Camera.pos.y) >> 4,
            .z = (pos.z - g_Camera.pos.z) >> 4,
        };
        while (ABS(pos.x - g_Camera.pos.x) > 0x8000
               || ABS(pos.y - g_Camera.pos.y) > 0x8000
               || ABS(pos.z - g_Camera.pos.z) > 0x8000) {
            pos.x -= step.x;
            pos.y -= step.y;
            pos.z -= step.z;
        }

        step.x = (pos.x - g_Camera.pos.x) >> 4;
        step.y = (pos.y - g_Camera.pos.y) >> 4;
        step.z = (pos.z - g_Camera.pos.z) >> 4;
        for (int32_t i = 0; i < 16; i++) {
            // Like the OG, any bounding-box match ends the search, even when
            // the point isn't strictly inside that room.
            Room_GetOutsideStatus(pos, nullptr, &room_num);
            if (room_num != NO_ROOM) {
                break;
            }
            pos.x -= step.x;
            pos.y -= step.y;
            pos.z -= step.z;
        }
    }

    bool los = false;
    if (room_num != NO_ROOM && (is_object || Room_Get(room_num)->flags.wind)) {
        GAME_VECTOR start = {
            .pos = g_Camera.pos.pos,
            .room_num = g_Camera.pos.room_num,
        };
        GAME_VECTOR target = { .pos = pos, .room_num = room_num };
        los = LOS_Check(&start, &target, false);
    }
    if (!los && is_object) {
        return;
    }

    // Project relative to Lara like the OG, halving the sun vector until it
    // is in representable range.
    XYZ_32 vec;
    if (is_object) {
        vec.x = pos.x - lara_item->pos.x;
        vec.y = pos.y - lara_item->pos.y;
        vec.z = pos.z - lara_item->pos.z;
    } else {
        vec.x = flare_pos.x - lara_item->pos.x;
        vec.y = flare_pos.y - lara_item->pos.y;
        vec.z = flare_pos.z - lara_item->pos.z;
        while (ABS(vec.x) > 0x7F00 || ABS(vec.y) > 0x7F00
               || ABS(vec.z) > 0x7F00) {
            vec.x >>= 1;
            vec.y >>= 1;
            vec.z >>= 1;
        }
    }

    const XYZ_32 world = {
        .x = lara_item->pos.x + vec.x,
        .y = lara_item->pos.y + vec.y,
        .z = lara_item->pos.z + vec.z,
    };
    float sx, sy;
    if (!M_ProjectToScreen(world, &sx, &sy)) {
        return;
    }

    const float center_y = Viewport_GetCenterY(VIEWPORT_GAME) * 640.0f
        / Viewport_GetWidth(VIEWPORT_GAME);
    const float dx = sx - 320.0f;
    const float dy = sy - center_y;
    const int32_t dist = (int32_t)sqrtf(SQUARE(dx) + SQUARE(dy));
    if (dist >= M_MAX_CENTER_DIST) {
        return;
    }
    const int32_t intensity = M_MAX_CENTER_DIST - dist;

    if (los) {
        const int32_t flash = intensity - M_FLASH_THRESHOLD;
        if (flash > 0) {
            m_FlashFader = M_FLASH_FADER_MAX;
            m_FlashColor.r = color.r * flash / M_MAX_CENTER_DIST;
            m_FlashColor.g = color.g * flash / M_MAX_CENTER_DIST;
            m_FlashColor.b = color.b * flash / M_MAX_CENTER_DIST;
        }
    }

    for (size_t i = is_object ? 1 : 0;
         i < sizeof(m_Ghosts) / sizeof(m_Ghosts[0]); i++) {
        const M_GHOST *const ghost = &m_Ghosts[i];
        int32_t r, g, b;
        if (i == 0) {
            // The sun glare flickers and ignores the center proximity.
            r = ghost->color.r + (Random_GetDraw() & 8);
            g = ghost->color.g;
            b = ghost->color.b + (Random_GetDraw() & 8);
        } else {
            r = intensity * ghost->color.r / M_MAX_CENTER_DIST;
            g = intensity * ghost->color.g / M_MAX_CENTER_DIST;
            b = intensity * ghost->color.b / M_MAX_CENTER_DIST;
        }
        r = (color.r * r) >> 8;
        g = (color.g * g) >> 8;
        b = (color.b * b) >> 8;

        if ((r | g | b) != 0) {
            const int32_t sprite_idx = M_GetSpriteIndex(ghost->sprite);
            if (sprite_idx < 0) {
                return;
            }
            // The sun disk (i == 0) sits at the OG's far plane so scene
            // geometry occludes it; the ghost trail stays in front.
            const bool depth_test = i == 0;
            const double view_dist =
                depth_test ? M_GetSunDiskViewDist() : M_GHOST_VIEW_DIST;
            M_StageGhost(
                sx - dx * ghost->line_pos / 16.0f,
                sy - dy * ghost->line_pos / 16.0f, ghost->size * 4.0f,
                (RGBA_8888) { r, g, b, 255 }, sprite_idx, depth_test,
                view_dist);
        }

        // Without a clear line of sight only the sun glare shows through.
        if (!los) {
            return;
        }
    }
}

static void M_StageFlash(void)
{
    if (m_FlashFader <= 0) {
        return;
    }
    const RGBA_8888 color = {
        .r = m_FlashColor.r * m_FlashFader / M_FLASH_FADER_MAX,
        .g = m_FlashColor.g * m_FlashFader / M_FLASH_FADER_MAX,
        .b = m_FlashColor.b * m_FlashFader / M_FLASH_FADER_MAX,
        .a = 255,
    };
    // A single flat additive billboard covering the whole screen.
    const float half_w = (float)((double)Viewport_GetWidth(VIEWPORT_GAME) / 2.0
                                 * M_GHOST_VIEW_DIST / g_PhdPersp);
    const float half_h = (float)((double)Viewport_GetHeight(VIEWPORT_GAME) / 2.0
                                 * M_GHOST_VIEW_DIST / g_PhdPersp);

    const MATRIX *const m = &g_ViewMatrix;
    const double w2v = (double)(1 << W2V_SHIFT);
    const XYZ_32 anchor = {
        .x = g_ViewPos.x + (int32_t)(m->_20 * M_GHOST_VIEW_DIST / w2v),
        .y = g_ViewPos.y + (int32_t)(m->_21 * M_GHOST_VIEW_DIST / w2v),
        .z = g_ViewPos.z + (int32_t)(m->_22 * M_GHOST_VIEW_DIST / w2v),
    };
    const XYZ_32 world_pos[4] = { anchor, anchor, anchor, anchor };
    const float disp[4][2] = {
        { -half_w, -half_h },
        { -half_w, half_h },
        { half_w, half_h },
        { half_w, -half_h },
    };
    const RGBA_8888 colors[4] = { color, color, color, color };
    OutputSource_PolyFX_StageQuadExtDepth(
        -1, world_pos, disp, colors,
        VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_BILLBOARD
            | VERT_ABS_SPRITE,
        -(float)M_GHOST_VIEW_DIST, DRAW_BLEND_ADD);
}

void Output_LensFlares_Reset(void)
{
    m_SunEnabled = false;
    m_SunPos = (XYZ_32) {};
    m_SunColor = (RGB_888) {};
    m_FlashFader = 0;
    m_FlashColor = (RGB_888) {};
}

void Output_LensFlares_SetSun(const XYZ_32 pos, const RGB_888 color)
{
    m_SunEnabled = true;
    m_SunPos = pos;
    m_SunColor = color;
}

void Output_LensFlares_Update(void)
{
    if (m_FlashFader > 0) {
        m_FlashFader -= 2;
        CLAMPL(m_FlashFader, 0);
    }
}

void Output_LensFlares_Draw(void)
{
    if (m_SunEnabled) {
        M_StageFlare(m_SunPos, NO_ROOM, m_SunColor, false);
    }
    M_StageFlash();
}

bool Output_LensFlares_DrawObject(const ITEM *const item)
{
    M_StageFlare(item->pos, item->room_num, COLOR_RGB_888_WHITE, true);
    return true;
}
