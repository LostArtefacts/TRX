#include <trx/game/rope.h>

#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/game/state.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/rooms.h>

#include <math.h>
#include <string.h>

#define M_DEFAULT_SEGMENT_LENGTH 128
#define M_PENDULUM_GRAVITY 0x60000
#define M_NODE_GRAVITY 0x30000
#define M_LARA_HAND_OFFSET (-112)
#define M_HALF_WIDTH 24.0f

// OG's DrawRope modulates the rope sprite by a flat 0xFF7F7F7F, i.e. half
// brightness. The TR4 shader path reads an unlit vertex color as a prelit
// value on the "128 = neutral" scale and doubles it, so OG's 0x7F modulate
// has to be halved here to survive that doubling: 0x40/255 * 255/128 = 0.5.
// Storing 0x7F instead leaves the rope at full bright, ignoring the room.
static const RGBA_8888 m_RopeColor = {
    .r = 0x40,
    .g = 0x40,
    .b = 0x40,
    .a = 0xFF,
};

static ROPE m_Ropes[MAX_ROPES] = {};
static int16_t m_RopeItems[MAX_ROPES] = {};
static int32_t m_RopeCount = 0;
static ROPE_PENDULUM m_Pendulum = {};
static ROPE_PENDULUM m_NullPendulum = {};

static XYZ_32 *M_Normalise(XYZ_32 *v);
static void M_Mul(const XYZ_32 *v, int32_t scale, XYZ_32 *d);
static int32_t M_DotProduct(const XYZ_32 *a, const XYZ_32 *b);
static void M_CrossProduct(const XYZ_32 *a, const XYZ_32 *b, XYZ_32 *n);
static void M_GetMatrixAngles(const int32_t m[9], int16_t dest[3]);
static void M_ModelRigid(
    const XYZ_32 *pa, const XYZ_32 *pb, XYZ_32 *va, XYZ_32 *vb,
    int32_t rlength);
static void M_ModelRigidRope(
    const XYZ_32 *pa, const XYZ_32 *pb, XYZ_32 *va, XYZ_32 *vb,
    int32_t rlength);
static void M_SetPendulumPoint(ROPE *rope, int32_t node);

static XYZ_32 *M_Normalise(XYZ_32 *const v)
{
    const int32_t x = v->x >> 16;
    const int32_t y = v->y >> 16;
    const int32_t z = v->z >> 16;
    if (x == 0 && y == 0 && z == 0) {
        return v;
    }

    const int32_t d = ABS(SQUARE(x) + SQUARE(y) + SQUARE(z));
    const int32_t mod = 65536 / (int32_t)Math_Sqrt(d);
    v->x = ((int64_t)mod * v->x) >> 16;
    v->y = ((int64_t)mod * v->y) >> 16;
    v->z = ((int64_t)mod * v->z) >> 16;
    return v;
}

static void M_Mul(const XYZ_32 *const v, const int32_t scale, XYZ_32 *const d)
{
    d->x = (scale * v->x) >> W2V_SHIFT;
    d->y = (scale * v->y) >> W2V_SHIFT;
    d->z = (scale * v->z) >> W2V_SHIFT;
}

static int32_t M_DotProduct(const XYZ_32 *const a, const XYZ_32 *const b)
{
    return (a->x * b->x + a->y * b->y + a->z * b->z) >> W2V_SHIFT;
}

static void M_CrossProduct(
    const XYZ_32 *const a, const XYZ_32 *const b, XYZ_32 *const n)
{
    const XYZ_32 t = {
        .x = a->y * b->z - a->z * b->y,
        .y = a->z * b->x - a->x * b->z,
        .z = a->x * b->y - a->y * b->x,
    };
    n->x = t.x >> W2V_SHIFT;
    n->y = t.y >> W2V_SHIFT;
    n->z = t.z >> W2V_SHIFT;
}

// Math_Atan's tangent table index overflows for arguments that exceed
// 16 bits; OG's phd_atan halves both arguments until the smaller one
// fits. Reproduce that here so large inputs stay bit-exact.
static int32_t M_Atan(int32_t x, int32_t y)
{
    const int32_t sign_x = x < 0 ? -1 : 1;
    const int32_t sign_y = y < 0 ? -1 : 1;
    x = ABS(x);
    y = ABS(y);
    while (MIN(x, y) > 0x7FFF) {
        x >>= 1;
        y >>= 1;
    }
    return Math_Atan(sign_x * x, sign_y * y);
}

// m is a row-major 3x3 rotation matrix scaled by (1 << 12).
static void M_GetMatrixAngles(const int32_t m[9], int16_t dest[3])
{
    const int16_t pitch_raw =
        (int16_t)M_Atan(Math_Sqrt(SQUARE(m[8]) + SQUARE(m[2])), m[5]);
    const int16_t pitch =
        (m[5] >= 0 && pitch_raw > 0) || (m[5] < 0 && pitch_raw < 0) ? -pitch_raw
                                                                    : pitch_raw;

    const int16_t yaw = (int16_t)M_Atan(m[8], m[2]);
    const int32_t sy = Math_Sin(yaw);
    const int32_t cy = Math_Cos(yaw);
    const int16_t roll =
        (int16_t)M_Atan(m[0] * cy - m[6] * sy, m[7] * sy - m[1] * cy);

    dest[0] = pitch;
    dest[1] = yaw;
    dest[2] = roll;
}

static void M_ModelRigid(
    const XYZ_32 *const pa, const XYZ_32 *const pb, XYZ_32 *const va,
    XYZ_32 *const vb, const int32_t rlength)
{
    XYZ_32 d = {
        .x = (pb->x - pa->x) + (vb->x - va->x),
        .y = (pb->y - pa->y) + (vb->y - va->y),
        .z = (pb->z - pa->z) + (vb->z - va->z),
    };
    const int32_t length = Math_Sqrt(
        ABS(SQUARE(d.x >> (W2V_SHIFT + 2)) + SQUARE(d.y >> (W2V_SHIFT + 2))
            + SQUARE(d.z >> (W2V_SHIFT + 2))));
    const int32_t scale = ((length << (W2V_SHIFT + 2)) - rlength) >> 1;
    M_Normalise(&d);

    const XYZ_32 delta = {
        .x = ((int64_t)scale * d.x) >> (W2V_SHIFT + 2),
        .y = ((int64_t)scale * d.y) >> (W2V_SHIFT + 2),
        .z = ((int64_t)scale * d.z) >> (W2V_SHIFT + 2),
    };
    va->x += delta.x;
    va->y += delta.y;
    va->z += delta.z;
    vb->x -= delta.x;
    vb->y -= delta.y;
    vb->z -= delta.z;
}

static void M_ModelRigidRope(
    const XYZ_32 *const pa, const XYZ_32 *const pb, XYZ_32 *const va,
    XYZ_32 *const vb, const int32_t rlength)
{
    XYZ_32 d = {
        .x = (pb->x - pa->x) + vb->x,
        .y = (pb->y - pa->y) + vb->y,
        .z = (pb->z - pa->z) + vb->z,
    };
    const int32_t length = Math_Sqrt(
        ABS(SQUARE(d.x >> (W2V_SHIFT + 2)) + SQUARE(d.y >> (W2V_SHIFT + 2))
            + SQUARE(d.z >> (W2V_SHIFT + 2))));
    const int32_t scale = (length << (W2V_SHIFT + 2)) - rlength;
    M_Normalise(&d);

    vb->x -= ((int64_t)scale * d.x) >> (W2V_SHIFT + 2);
    vb->y -= ((int64_t)scale * d.y) >> (W2V_SHIFT + 2);
    vb->z -= ((int64_t)scale * d.z) >> (W2V_SHIFT + 2);
}

static void M_SetPendulumPoint(ROPE *const rope, const int32_t node)
{
    m_Pendulum.pos = rope->segments[node];
    if (m_Pendulum.node == -1) {
        m_Pendulum.vel.x += rope->velocities[node].x;
        m_Pendulum.vel.y += rope->velocities[node].y;
        m_Pendulum.vel.z += rope->velocities[node].z;
    }
    m_Pendulum.rope = rope;
    m_Pendulum.node = node;
}

static void M_DrawRope(const ROPE *const rope, const int32_t sprite_idx)
{
    const double rate = (Interpolation_IsActive() && Game_IsPlaying())
        ? Interpolation_GetRate()
        : 1.0;
    XYZ_F points[ROPE_SEGMENTS];
    for (int32_t n = 0; n < ROPE_SEGMENTS; n++) {
        const XYZ_32 *const prev = &rope->prev_mesh_segments[n];
        const XYZ_32 *const cur = &rope->mesh_segments[n];
        points[n] = (XYZ_F) {
            .x = rope->pos.x
                + (prev->x + (cur->x - prev->x) * rate)
                    / (1 << (W2V_SHIFT + 2)),
            .y = rope->pos.y
                + (prev->y + (cur->y - prev->y) * rate)
                    / (1 << (W2V_SHIFT + 2)),
            .z = rope->pos.z
                + (prev->z + (cur->z - prev->z) * rate)
                    / (1 << (W2V_SHIFT + 2)),
        };
    }

    // Extrude a camera-facing ribbon along the rope nodes. OG builds the
    // equivalent quad strip in screen space with a constant world width.
    XYZ_F offsets[ROPE_SEGMENTS];
    for (int32_t n = 0; n < ROPE_SEGMENTS; n++) {
        const int32_t seg = MIN(n, ROPE_SEGMENTS - 2);
        const XYZ_F dir = {
            .x = points[seg + 1].x - points[seg].x,
            .y = points[seg + 1].y - points[seg].y,
            .z = points[seg + 1].z - points[seg].z,
        };
        const XYZ_F to_cam = {
            .x = points[n].x - g_Camera.pos.x,
            .y = points[n].y - g_Camera.pos.y,
            .z = points[n].z - g_Camera.pos.z,
        };
        // cross(to_cam, dir), not cross(dir, to_cam): OG's DrawRope takes the
        // screen-space perpendicular (-dy, dx) with Y pointing down, which
        // ends up on the opposite side of the rope from the world-space
        // cross. Getting this backwards mirrors the sprite's U axis and the
        // rope's weave leans the wrong way.
        XYZ_F perp = {
            .x = to_cam.y * dir.z - to_cam.z * dir.y,
            .y = to_cam.z * dir.x - to_cam.x * dir.z,
            .z = to_cam.x * dir.y - to_cam.y * dir.x,
        };
        const float length =
            sqrtf(SQUARE(perp.x) + SQUARE(perp.y) + SQUARE(perp.z));
        if (length < 1.0f) {
            offsets[n] = (XYZ_F) { .x = M_HALF_WIDTH, .y = 0.0f, .z = 0.0f };
            continue;
        }
        offsets[n] = (XYZ_F) {
            .x = perp.x * M_HALF_WIDTH / length,
            .y = perp.y * M_HALF_WIDTH / length,
            .z = perp.z * M_HALF_WIDTH / length,
        };
    }

    for (int32_t n = 0; n < ROPE_SEGMENTS - 1; n++) {
        // The sprite's V axis must run along the rope like in OG's
        // DrawRope, so the ribbon crosses the rope between corners 0-1.
        const XYZ_32 quad[4] = {
            {
                .x = points[n].x - offsets[n].x,
                .y = points[n].y - offsets[n].y,
                .z = points[n].z - offsets[n].z,
            },
            {
                .x = points[n].x + offsets[n].x,
                .y = points[n].y + offsets[n].y,
                .z = points[n].z + offsets[n].z,
            },
            {
                .x = points[n + 1].x + offsets[n + 1].x,
                .y = points[n + 1].y + offsets[n + 1].y,
                .z = points[n + 1].z + offsets[n + 1].z,
            },
            {
                .x = points[n + 1].x - offsets[n + 1].x,
                .y = points[n + 1].y - offsets[n + 1].y,
                .z = points[n + 1].z - offsets[n + 1].z,
            },
        };
        const RGBA_8888 colors[4] = { m_RopeColor, m_RopeColor, m_RopeColor,
                                      m_RopeColor };
        OutputSource_PolyFX_StageSpriteQuadWorld(
            sprite_idx, quad, colors, DRAW_BLEND);
    }
}

void Rope_Reset(void)
{
    memset(m_Ropes, 0, sizeof(m_Ropes));
    memset(m_RopeItems, 0, sizeof(m_RopeItems));
    m_RopeCount = 0;
    memset(&m_Pendulum, 0, sizeof(m_Pendulum));
    memset(&m_NullPendulum, 0, sizeof(m_NullPendulum));
}

void Rope_Create(const int16_t item_num)
{
    // Item_Initialise() also runs on savegame load; reuse the slot then.
    int32_t rope_num = Rope_GetIndexByItem(item_num);
    if (rope_num == NO_ROPE) {
        if (m_RopeCount >= MAX_ROPES) {
            return;
        }
        rope_num = m_RopeCount;
        m_RopeItems[rope_num] = item_num;
        m_RopeCount++;
    }

    const ITEM *const item = Item_Get(item_num);
    int16_t room_num = item->room_num;
    XYZ_32 pos = item->pos;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    pos.y = Room_GetCeiling(sector, pos);

    ROPE *const rope = &m_Ropes[rope_num];
    memset(rope, 0, sizeof(*rope));
    rope->pos = pos;
    rope->segment_length = M_DEFAULT_SEGMENT_LENGTH << 16;

    XYZ_32 dir = { .x = 0, .y = 0x4000, .z = 0 };
    dir.x <<= W2V_SHIFT + 2;
    dir.y <<= W2V_SHIFT + 2;
    dir.z <<= W2V_SHIFT + 2;
    M_Normalise(&dir);

    for (int32_t n = 0; n < ROPE_SEGMENTS; n++) {
        rope->segments[n].x =
            ((int64_t)(rope->segment_length * n) * dir.x) >> (W2V_SHIFT + 2);
        rope->segments[n].y =
            ((int64_t)(rope->segment_length * n) * dir.y) >> (W2V_SHIFT + 2);
        rope->segments[n].z =
            ((int64_t)(rope->segment_length * n) * dir.z) >> (W2V_SHIFT + 2);
        rope->velocities[n] = (XYZ_32) {};
    }
    rope->active = false;
}

ROPE *Rope_Get(const int32_t rope_num)
{
    if (rope_num < 0 || rope_num >= m_RopeCount) {
        return nullptr;
    }
    return &m_Ropes[rope_num];
}

int32_t Rope_GetIndexByItem(const int16_t item_num)
{
    for (int32_t i = 0; i < m_RopeCount; i++) {
        if (m_RopeItems[i] == item_num) {
            return i;
        }
    }
    return NO_ROPE;
}

ROPE_PENDULUM *Rope_GetPendulum(void)
{
    return &m_Pendulum;
}

void Rope_Calculate(ROPE *const rope)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ROPE *const lara_rope =
        lara->rope.index != NO_ROPE ? Rope_Get(lara->rope.index) : nullptr;

    ROPE_PENDULUM *pendulum;
    bool set_flag = false;
    if (rope == lara_rope) {
        pendulum = &m_Pendulum;
        if (m_Pendulum.node != lara->rope.segment + 1) {
            M_SetPendulumPoint(rope, lara->rope.segment + 1);
            set_flag = true;
        }
    } else {
        pendulum = &m_NullPendulum;
        if (lara->rope.index == NO_ROPE && m_Pendulum.rope != nullptr) {
            for (int32_t n = 0; n < m_Pendulum.node; n++) {
                m_Pendulum.rope->velocities[n] =
                    m_Pendulum.rope->velocities[m_Pendulum.node];
            }
            m_Pendulum.rope = nullptr;
            m_Pendulum.node = -1;
            m_Pendulum.pos = (XYZ_32) {};
            m_Pendulum.vel = (XYZ_32) {};
        }
    }

    if (lara->rope.index != NO_ROPE) {
        XYZ_32 dir = {
            .x = pendulum->pos.x - rope->segments[0].x,
            .y = pendulum->pos.y - rope->segments[0].y,
            .z = pendulum->pos.z - rope->segments[0].z,
        };
        M_Normalise(&dir);

        for (int32_t n = pendulum->node; n >= 0; n--) {
            // OG reads MeshSegment[-1] for node 0, which lands on a
            // never-written field that is always zero.
            const XYZ_32 base =
                n > 0 ? rope->mesh_segments[n - 1] : (XYZ_32) {};
            rope->segments[n].x = base.x
                + (((int64_t)rope->segment_length * dir.x) >> (W2V_SHIFT + 2));
            rope->segments[n].y = base.y
                + (((int64_t)rope->segment_length * dir.y) >> (W2V_SHIFT + 2));
            rope->segments[n].z = base.z
                + (((int64_t)rope->segment_length * dir.z) >> (W2V_SHIFT + 2));
            rope->velocities[n] = (XYZ_32) {};
        }

        if (set_flag) {
            const XYZ_32 delta = {
                .x = pendulum->pos.x - rope->segments[pendulum->node].x,
                .y = pendulum->pos.y - rope->segments[pendulum->node].y,
                .z = pendulum->pos.z - rope->segments[pendulum->node].z,
            };
            rope->segments[pendulum->node] = pendulum->pos;

            for (int32_t n = pendulum->node; n < ROPE_SEGMENTS; n++) {
                rope->segments[n].x -= delta.x;
                rope->segments[n].y -= delta.y;
                rope->segments[n].z -= delta.z;
                rope->velocities[n] = (XYZ_32) {};
            }
        }

        M_ModelRigidRope(
            &rope->segments[0], &pendulum->pos, &rope->velocities[0],
            &pendulum->vel, rope->segment_length * pendulum->node);
        pendulum->vel.y += M_PENDULUM_GRAVITY;
        pendulum->pos.x += pendulum->vel.x;
        pendulum->pos.y += pendulum->vel.y;
        pendulum->pos.z += pendulum->vel.z;
        pendulum->vel.x -= pendulum->vel.x >> 8;
        pendulum->vel.z -= pendulum->vel.z >> 8;
    }

    for (int32_t n = pendulum->node; n < ROPE_SEGMENTS - 1; n++) {
        M_ModelRigid(
            &rope->segments[n], &rope->segments[n + 1], &rope->velocities[n],
            &rope->velocities[n + 1], rope->segment_length);
    }

    for (int32_t n = 0; n < ROPE_SEGMENTS; n++) {
        rope->segments[n].x += rope->velocities[n].x;
        rope->segments[n].y += rope->velocities[n].y;
        rope->segments[n].z += rope->velocities[n].z;
    }

    for (int32_t n = pendulum->node; n < ROPE_SEGMENTS; n++) {
        rope->velocities[n].y += M_NODE_GRAVITY;
        if (pendulum->rope != nullptr) {
            rope->velocities[n].x -= rope->velocities[n].x >> 4;
            rope->velocities[n].z -= rope->velocities[n].z >> 4;
        } else {
            rope->velocities[n].x -= rope->velocities[n].x >> 7;
            rope->velocities[n].z -= rope->velocities[n].z >> 7;
        }
    }

    rope->segments[0] = (XYZ_32) {};
    rope->velocities[0] = (XYZ_32) {};

    for (int32_t n = 0; n < ROPE_SEGMENTS - 1; n++) {
        rope->normalised_segments[n].x =
            rope->segments[n + 1].x - rope->segments[n].x;
        rope->normalised_segments[n].y =
            rope->segments[n + 1].y - rope->segments[n].y;
        rope->normalised_segments[n].z =
            rope->segments[n + 1].z - rope->segments[n].z;
        M_Normalise(&rope->normalised_segments[n]);
    }

    if (rope != lara_rope) {
        rope->mesh_segments[0] = rope->segments[0];
        rope->mesh_segments[1].x = rope->segments[0].x
            + (((int64_t)rope->segment_length * rope->normalised_segments[0].x)
               >> 16);
        rope->mesh_segments[1].y = rope->segments[0].y
            + (((int64_t)rope->segment_length * rope->normalised_segments[0].y)
               >> 16);
        rope->mesh_segments[1].z = rope->segments[0].z
            + (((int64_t)rope->segment_length * rope->normalised_segments[0].z)
               >> 16);

        for (int32_t n = 2; n < ROPE_SEGMENTS; n++) {
            rope->mesh_segments[n].x = rope->mesh_segments[n - 1].x
                + (((int64_t)rope->segment_length
                    * rope->normalised_segments[n - 1].x)
                   >> (W2V_SHIFT + 2));
            rope->mesh_segments[n].y = rope->mesh_segments[n - 1].y
                + (((int64_t)rope->segment_length
                    * rope->normalised_segments[n - 1].y)
                   >> (W2V_SHIFT + 2));
            rope->mesh_segments[n].z = rope->mesh_segments[n - 1].z
                + (((int64_t)rope->segment_length
                    * rope->normalised_segments[n - 1].z)
                   >> (W2V_SHIFT + 2));
        }
    } else {
        const int32_t node = pendulum->node;
        rope->mesh_segments[node] = rope->segments[node];
        rope->mesh_segments[node + 1].x = rope->segments[node].x
            + (((int64_t)rope->segment_length
                * rope->normalised_segments[node].x)
               >> (W2V_SHIFT + 2));
        rope->mesh_segments[node + 1].y = rope->segments[node].y
            + (((int64_t)rope->segment_length
                * rope->normalised_segments[node].y)
               >> (W2V_SHIFT + 2));
        rope->mesh_segments[node + 1].z = rope->segments[node].z
            + (((int64_t)rope->segment_length
                * rope->normalised_segments[node].z)
               >> (W2V_SHIFT + 2));

        for (int32_t n = node + 1; n < ROPE_SEGMENTS - 1; n++) {
            rope->mesh_segments[n + 1].x = rope->mesh_segments[n].x
                + (((int64_t)rope->segment_length
                    * rope->normalised_segments[n].x)
                   >> (W2V_SHIFT + 2));
            rope->mesh_segments[n + 1].y = rope->mesh_segments[n].y
                + (((int64_t)rope->segment_length
                    * rope->normalised_segments[n].y)
                   >> (W2V_SHIFT + 2));
            rope->mesh_segments[n + 1].z = rope->mesh_segments[n].z
                + (((int64_t)rope->segment_length
                    * rope->normalised_segments[n].z)
                   >> (W2V_SHIFT + 2));
        }

        for (int32_t n = 0; n < node; n++) {
            rope->mesh_segments[n] = rope->segments[n];
        }
    }
}

void Rope_GetPos(
    const ROPE *const rope, const int32_t rel_pos, XYZ_32 *const out_pos)
{
    const int32_t segment = rel_pos >> 7;
    const int32_t frac = rel_pos & 0x7F;
    out_pos->x =
        ((rope->normalised_segments[segment].x * frac) >> (W2V_SHIFT + 2))
        + (rope->mesh_segments[segment].x >> (W2V_SHIFT + 2)) + rope->pos.x;
    out_pos->y =
        ((rope->normalised_segments[segment].y * frac) >> (W2V_SHIFT + 2))
        + (rope->mesh_segments[segment].y >> (W2V_SHIFT + 2)) + rope->pos.y;
    out_pos->z =
        ((rope->normalised_segments[segment].z * frac) >> (W2V_SHIFT + 2))
        + (rope->mesh_segments[segment].z >> (W2V_SHIFT + 2)) + rope->pos.z;
}

int32_t Rope_NodeCollision(
    const ROPE *const rope, const XYZ_32 pos, const int32_t radius)
{
    for (int32_t i = 0; i < ROPE_SEGMENTS - 2; i++) {
        if (pos.y <= rope->pos.y + (rope->mesh_segments[i].y >> (W2V_SHIFT + 2))
            || pos.y >= rope->pos.y
                    + (rope->mesh_segments[i + 1].y >> (W2V_SHIFT + 2))) {
            continue;
        }

        const int32_t dx = pos.x
            - ((rope->mesh_segments[i + 1].x + rope->mesh_segments[i].x) >> 17)
            - rope->pos.x;
        const int32_t dy = pos.y
            - ((rope->mesh_segments[i + 1].y + rope->mesh_segments[i].y) >> 17)
            - rope->pos.y;
        const int32_t dz = pos.z
            - ((rope->mesh_segments[i + 1].z + rope->mesh_segments[i].z) >> 17)
            - rope->pos.z;
        if (SQUARE(dx) + SQUARE(dy) + SQUARE(dz) < SQUARE(radius + 64)) {
            return i;
        }
    }

    return -1;
}

void Rope_SetPendulumVelocity(const int32_t x, const int32_t y, const int32_t z)
{
    int32_t vx = x;
    int32_t vy = y;
    int32_t vz = z;

    if (2 * (m_Pendulum.node >> 1) < ROPE_SEGMENTS) {
        const int32_t scale =
            4096 / (ROPE_SEGMENTS - 2 * (m_Pendulum.node >> 1)) * 256;
        vx = ((int64_t)scale * vx) >> (W2V_SHIFT + 2);
        vy = ((int64_t)scale * vy) >> (W2V_SHIFT + 2);
        vz = ((int64_t)scale * vz) >> (W2V_SHIFT + 2);
    }

    m_Pendulum.vel.x += vx;
    m_Pendulum.vel.y += vy;
    m_Pendulum.vel.z += vz;
}

void Rope_AlignLara(ITEM *const item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    const ROPE *const rope = Rope_Get(lara->rope.index);
    if (rope == nullptr) {
        return;
    }

    const XYZ_32 up = { .x = 4096, .y = 0, .z = 0 };
    const int16_t frame_offset_y = Item_GetBestFrame(item)->offset.y;
    const int16_t rope_angle = (int16_t)(lara->rope.y_rot - 16380);
    const int32_t i = lara->rope.segment;

    XYZ_32 pos_a;
    XYZ_32 pos_b;
    Rope_GetPos(rope, (i - 1) * 128 + frame_offset_y, &pos_a);
    Rope_GetPos(rope, (i - 1) * 128 + frame_offset_y - 192, &pos_b);

    XYZ_32 u = {
        .x = (pos_a.x - pos_b.x) << (W2V_SHIFT + 2),
        .y = (pos_a.y - pos_b.y) << (W2V_SHIFT + 2),
        .z = (pos_a.z - pos_b.z) << (W2V_SHIFT + 2),
    };
    M_Normalise(&u);
    u.x >>= 2;
    u.y >>= 2;
    u.z >>= 2;

    XYZ_32 v;
    M_Mul(&u, M_DotProduct(&up, &u), &v);
    v.x = up.x - v.x;
    v.y = up.y - v.y;
    v.z = up.z - v.z;

    XYZ_32 v1 = v;
    XYZ_32 v2 = v;
    XYZ_32 n2 = u;
    M_Mul(&v1, Math_Cos(rope_angle), &v1);
    M_Mul(&n2, M_DotProduct(&n2, &v), &n2);
    M_Mul(&n2, 4096 - Math_Cos(rope_angle), &n2);
    M_CrossProduct(&u, &v, &v2);
    M_Mul(&v2, Math_Sin(rope_angle), &v2);
    n2.x += v1.x;
    n2.y += v1.y;
    n2.z += v1.z;

    v.x = (n2.x + v2.x) << (W2V_SHIFT + 2);
    v.y = (n2.y + v2.y) << (W2V_SHIFT + 2);
    v.z = (n2.z + v2.z) << (W2V_SHIFT + 2);
    M_Normalise(&v);
    v.x >>= 2;
    v.y >>= 2;
    v.z >>= 2;

    XYZ_32 n;
    M_CrossProduct(&u, &v, &n);
    n.x <<= W2V_SHIFT + 2;
    n.y <<= W2V_SHIFT + 2;
    n.z <<= W2V_SHIFT + 2;
    M_Normalise(&n);
    n.x >>= 2;
    n.y >>= 2;
    n.z >>= 2;

    const int32_t m[9] = {
        n.x, u.x, v.x, //
        n.y, u.y, v.y, //
        n.z, u.z, v.z, //
    };
    int16_t angles[3];
    M_GetMatrixAngles(m, angles);

    item->pos.x = rope->pos.x + (rope->mesh_segments[i].x >> (W2V_SHIFT + 2));
    item->pos.y = rope->pos.y + (rope->mesh_segments[i].y >> (W2V_SHIFT + 2))
        + lara->rope.offset;
    item->pos.z = rope->pos.z + (rope->mesh_segments[i].z >> (W2V_SHIFT + 2));

    Matrix_PushUnit();
    Matrix_Rot16((XYZ_16) { .x = angles[0], .y = angles[1], .z = angles[2] });
    item->pos.x += (M_LARA_HAND_OFFSET * g_MatrixPtr->_02) >> W2V_SHIFT;
    item->pos.y += (M_LARA_HAND_OFFSET * g_MatrixPtr->_12) >> W2V_SHIFT;
    item->pos.z += (M_LARA_HAND_OFFSET * g_MatrixPtr->_22) >> W2V_SHIFT;
    Matrix_Pop();

    item->rot.x = angles[0];
    item->rot.y = angles[1];
    item->rot.z = angles[2];
}

// Like OG's DrawRopeList, ropes draw in a global pass independent of the
// drawn-rooms set: their segments may hang through rooms other than the
// one that owns the rope item.
void Rope_DrawAll(void)
{
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_ROPE);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    for (int32_t i = 0; i < m_RopeCount; i++) {
        if (m_Ropes[i].active) {
            M_DrawRope(&m_Ropes[i], sprite_idx);
        }
    }
}
