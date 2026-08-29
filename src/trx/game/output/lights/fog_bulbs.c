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
    FOG_BULB data;
    float sqrad, inv_sqrad;
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
// The order the level's bulbs are staged in, nearest camera first. The bulbs
// themselves stay where the level put them, so that a script holding one keeps
// hold of the same bulb as the camera moves.
static int32_t m_StaticOrder[M_MAX_STATIC_BULBS];
static HANDLE_EPOCH m_StaticEpoch = {};
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

    const XYZ_F world_pos = {
        (float)bulb->data.pos.x,
        (float)bulb->data.pos.y,
        (float)bulb->data.pos.z,
    };
    const XYZ_F cam = {
        (float)g_Camera.pos.pos.x,
        (float)g_Camera.pos.pos.y,
        (float)g_Camera.pos.pos.z,
    };
    const XYZ_F d = {
        world_pos.x - cam.x,
        world_pos.y - cam.y,
        world_pos.z - cam.z,
    };
    bulb->sq_cam_dist = SQUARE(d.x) + SQUARE(d.y) + SQUARE(d.z);
    if (bulb->sq_cam_dist > (float)M_MAX_SQ_DIST) {
        return;
    }

    const XYZ_F pos = M_ViewTransform(world_pos);
    // Rough frustum cull (the OG uses S_GetObjectBounds): reject bulbs
    // entirely behind the camera.
    if (pos.z < -bulb->data.radius) {
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
        bulb->data.radius * dir.x + pos.x,
        bulb->data.radius * dir.y + pos.y,
        bulb->data.radius * dir.z + pos.z,
    };
}

// Port of the OG ControlFXBulb (polyinsert.cpp:379), sans the smoke spawns
// (those are the responsibility of what triggered the bulb).
static void M_ControlTimedBulb(M_BULB *const bulb)
{
    if (bulb->timer > 0) {
        bulb->timer--;
        bulb->data.radius += (float)((Random_GetDraw() - 0x100) & 0x1FF);
        if (bulb->data.radius > (float)bulb->fx_rad) {
            bulb->data.radius =
                (float)(bulb->fx_rad + (Random_GetDraw() & 0xFF));
        }
    } else {
        bulb->timer--;
        if (bulb->timer < -30) {
            bulb->timer = 0;
        }
        bulb->data.radius -= 255.0f;
        if (bulb->data.radius < 0.0f) {
            bulb->data.radius = 0.0f;
            bulb->active = false;
        }
    }

    bulb->sqrad = SQUARE(bulb->data.radius);
    bulb->inv_sqrad = 1.0f / bulb->sqrad;
}

static int M_CompareBulbDist(const void *const a, const void *const b)
{
    const M_BULB *const bulb_a = &m_Static[*(const int32_t *)a];
    const M_BULB *const bulb_b = &m_Static[*(const int32_t *)b];
    if (bulb_a->sq_cam_dist > bulb_b->sq_cam_dist) {
        return 1;
    }
    if (bulb_a->sq_cam_dist < bulb_b->sq_cam_dist) {
        return -1;
    }
    return 0;
}

// The color a bulb is drawn in: its own where a script gave it one, and the
// fog color in force where none was given.
static RGB_888 M_GetColor(const FOG_BULB *const bulb)
{
    return bulb->has_own_color ? bulb->color : Level_GetFogTint();
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
    const RGB_888 color = M_GetColor(&bulb->data);
    dst->bulbs[i].color[0] = color.r / 255.0f;
    dst->bulbs[i].color[1] = color.g / 255.0f;
    dst->bulbs[i].color[2] = color.b / 255.0f;
    dst->bulbs[i].color[3] = (float)bulb->data.density;
    dst->bulbs[i].params[0] = bulb->inv_sqrad;
    dst->bulbs[i].params[1] = is_fx ? 1.0f : 0.0f;
    return true;
}

void Output_FogBulbs_ResetStatic(void)
{
    m_StaticCount = 0;
    Handle_EpochBump(&m_StaticEpoch);
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

void Output_FogBulbs_SetColor(FOG_BULB *const bulb, const RGB_888 *const color)
{
    bulb->has_own_color = color != nullptr;
    if (color != nullptr) {
        bulb->color = *color;
    }
}

void Output_FogBulbs_AddStatic(
    const XYZ_32 pos, const float radius, const int32_t density,
    const int16_t room_num)
{
    if (m_StaticCount >= M_MAX_STATIC_BULBS) {
        return;
    }

    m_Static[m_StaticCount++] = (M_BULB) {
        .data = {
            .pos = pos,
            .radius = radius,
            .density = density,
            .room_num = room_num,
        },
        .sqrad = SQUARE(radius),
        .inv_sqrad = 1.0f / SQUARE(radius),
    };
}

int32_t Output_FogBulbs_GetStaticCount(void)
{
    return m_StaticCount;
}

FOG_BULB *Output_FogBulbs_GetStatic(const int32_t idx)
{
    if (idx < 0 || idx >= m_StaticCount) {
        return nullptr;
    }
    return &m_Static[idx].data;
}

TRX_HANDLE Output_FogBulbs_GetStaticHandle(const int32_t idx)
{
    return Handle_EpochMint(&m_StaticEpoch, idx);
}

FOG_BULB *Output_FogBulbs_FromHandle(const TRX_HANDLE handle)
{
    if (!Handle_EpochIsLive(&m_StaticEpoch, handle)) {
        return nullptr;
    }
    return Output_FogBulbs_GetStatic(handle.id);
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
        .data = {
            .pos = pos,
            .radius = rad,
            .density = density,
            .color = color,
            .has_own_color = true,
        },
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
        .data = {
            .pos = pos,
            .density = density,
            .color = color,
            .has_own_color = true,
        },
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
                (float)m_Static[i].data.pos.x - (float)g_Camera.pos.pos.x,
                (float)m_Static[i].data.pos.y - (float)g_Camera.pos.pos.y,
                (float)m_Static[i].data.pos.z - (float)g_Camera.pos.pos.z,
            };
            m_Static[i].sq_cam_dist = SQUARE(d.x) + SQUARE(d.y) + SQUARE(d.z);
            m_StaticOrder[i] = i;
        }
        qsort(
            m_StaticOrder, m_StaticCount, sizeof(m_StaticOrder[0]),
            M_CompareBulbDist);

        int32_t n_active = 0;
        for (int32_t i = 0; i < m_StaticCount && n_active < M_MAX_ACTIVE_BULBS;
             i++) {
            M_BULB *const bulb = &m_Static[m_StaticOrder[i]];
            M_CreateFogPos(bulb);
            if (bulb->in_range && M_FillUniform(&fog, bulb, false)) {
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
