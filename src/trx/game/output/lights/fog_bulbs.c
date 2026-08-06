// Volumetric fog bulbs (port of the OG OmniEffect/OmniFog, polyinsert.cpp).
// The staging is the same for every game: put each bulb in view space, keep
// the ones the camera can see, and hand the shader what it reads. Only the
// bulbs themselves differ - TR4 levels carry them as room lights and the OG
// triggers timed ones, while a script's bulb lasts the frame it asked for.

#include <trx/game/output/lights/fog_bulbs.h>

#include <trx/core/utils.h>
#include <trx/game/camera/vars.h>
#include <trx/game/level/settings.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/random.h>
#include <trx/gl/utils.h>

#include <math.h>
#include <stdlib.h>

#define M_MAX_STATIC_BULBS 20
#define M_MAX_TIMED_BULBS 5
// A frame bulb takes a slot in the buffer the level's own bulbs fill, so
// holding more than the buffer shows would gain nothing.
#define M_MAX_FRAME_BULBS OUTPUT_MAX_FOG_BULBS
#define M_MAX_ACTIVE_BULBS 5
#define M_MAX_SQ_DIST 0x19000000 // = 20480^2, the OG CreateFogPos

typedef struct {
    XYZ_F world_pos;
    float rad, sqrad, inv_sqrad;
    int32_t density;
    RGB_888 color; // timed bulbs only; level bulbs use the fog color
    int32_t timer;
    int32_t fx_rad;
    bool active; // timed bulbs only
    // Per-frame view-space staging (the OG CreateFogPos):
    bool in_range;
    XYZ_F pos_view;
    XYZ_F edge_view;
    float view_dist;
    float sq_cam_dist;
} M_BULB;

static M_BULB m_Static[M_MAX_STATIC_BULBS];
static int32_t m_StaticCount = 0;
static M_BULB m_Timed[M_MAX_TIMED_BULBS];
static M_BULB m_Frame[M_MAX_FRAME_BULBS];
static int32_t m_FrameCount = 0;
// What the shader was last given, so that the games carrying no bulbs at all
// upload an empty buffer once rather than every frame.
static int32_t m_UploadedCount = -1;

static XYZ_F M_ViewTransform(const XYZ_F v)
{
    const MATRIX *const m = &g_ViewMatrix;
    const float s = 1.0f / (float)(1 << W2V_SHIFT);
    return (XYZ_F) {
        .x = (m->_00 * v.x + m->_01 * v.y + m->_02 * v.z + m->_03) * s,
        .y = (m->_10 * v.x + m->_11 * v.y + m->_12 * v.z + m->_13) * s,
        .z = (m->_20 * v.x + m->_21 * v.y + m->_22 * v.z + m->_23) * s,
    };
}

// Port of the OG CreateFogPos (polyinsert.cpp:320).
static void M_CreateFogPos(M_BULB *const bulb)
{
    bulb->in_range = false;

    const XYZ_F cam = {
        (float)g_Camera.pos.pos.x,
        (float)g_Camera.pos.pos.y,
        (float)g_Camera.pos.pos.z,
    };
    const XYZ_F d = {
        bulb->world_pos.x - cam.x,
        bulb->world_pos.y - cam.y,
        bulb->world_pos.z - cam.z,
    };
    bulb->sq_cam_dist = SQUARE(d.x) + SQUARE(d.y) + SQUARE(d.z);
    if (bulb->sq_cam_dist > (float)M_MAX_SQ_DIST) {
        return;
    }

    const XYZ_F pos = M_ViewTransform(bulb->world_pos);
    // Rough frustum cull (the OG uses S_GetObjectBounds): reject bulbs
    // entirely behind the camera.
    if (pos.z < -bulb->rad) {
        return;
    }

    bulb->in_range = true;
    bulb->pos_view = pos;
    const float len = sqrtf(SQUARE(pos.x) + SQUARE(pos.y) + SQUARE(pos.z));
    bulb->view_dist = len;
    XYZ_F dir = pos;
    if (len > 0.0f) {
        dir.x /= len;
        dir.y /= len;
        dir.z /= len;
    }
    bulb->edge_view = (XYZ_F) {
        bulb->rad * dir.x + pos.x,
        bulb->rad * dir.y + pos.y,
        bulb->rad * dir.z + pos.z,
    };
}

// Port of the OG ControlFXBulb (polyinsert.cpp:379), sans the smoke spawns
// (those are the responsibility of what triggered the bulb).
static void M_ControlTimedBulb(M_BULB *const bulb)
{
    if (bulb->timer > 0) {
        bulb->timer--;
        bulb->rad += (float)((Random_GetDraw() - 0x100) & 0x1FF);
        if (bulb->rad > (float)bulb->fx_rad) {
            bulb->rad = (float)(bulb->fx_rad + (Random_GetDraw() & 0xFF));
        }
    } else {
        bulb->timer--;
        if (bulb->timer < -30) {
            bulb->timer = 0;
        }
        bulb->rad -= 255.0f;
        if (bulb->rad < 0.0f) {
            bulb->rad = 0.0f;
            bulb->active = false;
        }
    }

    bulb->sqrad = SQUARE(bulb->rad);
    bulb->inv_sqrad = 1.0f / bulb->sqrad;
}

static int M_CompareBulbDist(const void *const a, const void *const b)
{
    const M_BULB *const bulb_a = a;
    const M_BULB *const bulb_b = b;
    if (bulb_a->sq_cam_dist > bulb_b->sq_cam_dist) {
        return 1;
    }
    if (bulb_a->sq_cam_dist < bulb_b->sq_cam_dist) {
        return -1;
    }
    return 0;
}

static bool M_FillUniform(
    OUTPUT_UNIFORM_FOG_BULBS *const dst, const M_BULB *const bulb,
    const bool is_fx)
{
    if (dst->count >= OUTPUT_MAX_FOG_BULBS) {
        return false;
    }
    const int32_t i = dst->count++;
    dst->bulbs[i].pos[0] = bulb->pos_view.x;
    dst->bulbs[i].pos[1] = bulb->pos_view.y;
    dst->bulbs[i].pos[2] = bulb->pos_view.z;
    dst->bulbs[i].pos[3] = bulb->view_dist;
    dst->bulbs[i].edge[0] = bulb->edge_view.x;
    dst->bulbs[i].edge[1] = bulb->edge_view.y;
    dst->bulbs[i].edge[2] = bulb->edge_view.z;
    dst->bulbs[i].edge[3] = bulb->sqrad;
    dst->bulbs[i].color[0] = bulb->color.r / 255.0f;
    dst->bulbs[i].color[1] = bulb->color.g / 255.0f;
    dst->bulbs[i].color[2] = bulb->color.b / 255.0f;
    dst->bulbs[i].color[3] = (float)bulb->density;
    dst->bulbs[i].params[0] = bulb->inv_sqrad;
    dst->bulbs[i].params[1] = is_fx ? 1.0f : 0.0f;
    return true;
}

void Output_FogBulbs_ResetStatic(void)
{
    m_StaticCount = 0;
}

void Output_FogBulbs_ResetUploadCache(void)
{
    m_UploadedCount = -1;
}

void Output_FogBulbs_Reset(void)
{
    Output_FogBulbs_ResetStatic();
    m_FrameCount = 0;
    for (int32_t i = 0; i < M_MAX_TIMED_BULBS; i++) {
        m_Timed[i].active = false;
    }
}

void Output_FogBulbs_AddStatic(
    const XYZ_F world_pos, const float radius, const RGB_888 color,
    const int32_t density)
{
    if (m_StaticCount >= M_MAX_STATIC_BULBS) {
        return;
    }

    m_Static[m_StaticCount++] = (M_BULB) {
        .world_pos = world_pos,
        .density = density,
        .color = color,
        .rad = radius,
        .sqrad = SQUARE(radius),
        .inv_sqrad = 1.0f / SQUARE(radius),
    };
}

void Output_FogBulbs_AddFrame(
    const XYZ_32 pos, const int32_t radius, const int32_t density,
    const RGB_888 color)
{
    if (m_FrameCount >= M_MAX_FRAME_BULBS || radius <= 0) {
        return;
    }

    const float rad = (float)radius;
    m_Frame[m_FrameCount++] = (M_BULB) {
        .world_pos = { (float)pos.x, (float)pos.y, (float)pos.z },
        .density = density,
        .color = color,
        .rad = rad,
        .sqrad = SQUARE(rad),
        .inv_sqrad = 1.0f / SQUARE(rad),
    };
}

void Output_FogBulbs_ResetFrame(void)
{
    m_FrameCount = 0;
}

// Port of the OG TriggerFXFogBulb (polyinsert.cpp:439).
void Output_FogBulbs_AddTimed(
    const XYZ_32 pos, const int32_t fx_rad, const int32_t density,
    const RGB_888 color)
{
    int32_t num = 0;
    while (m_Timed[num].active) {
        num++;
        if (num >= M_MAX_TIMED_BULBS) {
            return;
        }
    }

    m_Timed[num] = (M_BULB) {
        .world_pos = { (float)pos.x, (float)pos.y, (float)pos.z },
        .density = density,
        .color = color,
        .rad = 0.0f,
        .sqrad = 0.0f,
        .inv_sqrad = 0.0f,
        .timer = 50,
        .fx_rad = fx_rad,
        .active = true,
    };
}

void Output_FogBulbs_Animate(const int32_t num_frames)
{
    for (int32_t f = 0; f < num_frames; f++) {
        for (int32_t i = 0; i < M_MAX_TIMED_BULBS; i++) {
            if (m_Timed[i].active) {
                M_ControlTimedBulb(&m_Timed[i]);
            }
        }
    }
}

// Port of the OG InitialiseFogBulbs + CreateFXBulbs (polyinsert.cpp): sort
// the level bulbs by camera distance, keep the nearest ones in range, and
// upload everything for the fragment shader.
void Output_FogBulbs_PrepareScene(void)
{
    OUTPUT_UNIFORM_FOG_BULBS fog = {};

    // The level's own bulbs are staged first, so that a script asking for more
    // than the buffer holds takes the slots left over rather than the ones the
    // level was drawing in.
    //
    // Some levels disable fog bulbs entirely (the OG GF_TRAIN). The OG also
    // skips them while the inventory is up, which we deliberately don't.
    if (Level_AreFogBulbsEnabled()) {
        for (int32_t i = 0; i < M_MAX_TIMED_BULBS; i++) {
            M_BULB *const bulb = &m_Timed[i];
            if (!bulb->active || bulb->sqrad <= 0.0f) {
                continue;
            }
            M_CreateFogPos(bulb);
            if (bulb->in_range) {
                M_FillUniform(&fog, bulb, true);
            }
        }

        for (int32_t i = 0; i < m_StaticCount; i++) {
            const XYZ_F d = {
                m_Static[i].world_pos.x - g_Camera.pos.pos.x,
                m_Static[i].world_pos.y - g_Camera.pos.pos.y,
                m_Static[i].world_pos.z - g_Camera.pos.pos.z,
            };
            m_Static[i].sq_cam_dist = SQUARE(d.x) + SQUARE(d.y) + SQUARE(d.z);
        }
        qsort(m_Static, m_StaticCount, sizeof(M_BULB), M_CompareBulbDist);

        int32_t n_active = 0;
        for (int32_t i = 0; i < m_StaticCount && n_active < M_MAX_ACTIVE_BULBS;
             i++) {
            M_CreateFogPos(&m_Static[i]);
            if (m_Static[i].in_range
                && M_FillUniform(&fog, &m_Static[i], false)) {
                n_active++;
            }
        }
    }

    // A script's bulb is the script's business, so the setting that governs
    // the game's own bulbs does not reach it.
    for (int32_t i = 0; i < m_FrameCount; i++) {
        M_CreateFogPos(&m_Frame[i]);
        if (m_Frame[i].in_range) {
            M_FillUniform(&fog, &m_Frame[i], true);
        }
    }

    if (fog.count == 0 && m_UploadedCount == 0) {
        return;
    }
    m_UploadedCount = fog.count;

    const OUTPUT_UNIFORMS *const uniforms = Output_GetUniforms();
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->fog_bulbs);
    const size_t size = offsetof(OUTPUT_UNIFORM_FOG_BULBS, bulbs)
        + fog.count * sizeof(fog.bulbs[0]);
    TRX_GL_TRACK_SUBDATA(glBufferSubData, GL_UNIFORM_BUFFER, 0, size, &fog);
    TRX_GL_CheckError();
}
