#include <trx/game/fx/upv_wake.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/fx/common.h>
#include <trx/game/items/const.h>
#include <trx/game/items/manager.h>
#include <trx/game/output/sources/poly_fx.h>

#define M_MAX_POINTS 32
#define M_MAX_LIFE 64
#define M_MAX_SHADE 16

typedef struct {
    XYZ_32 pos[2];
    uint8_t life;
} M_POINT;

static M_POINT m_Points[M_MAX_POINTS][2] = {};
static uint8_t m_Shade = 0;
static uint8_t m_StartIndex = 0;
static int16_t m_OwnerNum = NO_ITEM;

static bool M_IsOwner(const ITEM *const item)
{
    return Item_GetIndex(item) == m_OwnerNum;
}

static RGBA_8888 M_GetColor(int32_t c)
{
    if (m_Shade < M_MAX_SHADE) {
        c = (c * m_Shade) / M_MAX_SHADE;
    }
    c = (c >> 1) << 3;
    CLAMPG(c, 255);
    return (RGBA_8888) { c, c, c, 255 };
}

static void M_Control(void)
{
    for (int32_t i = 0; i < M_MAX_POINTS; i++) {
        for (int32_t side = 0; side < 2; side++) {
            M_POINT *const pt = &m_Points[i][side];
            if (pt->life > 0) {
                pt->life--;
            }
        }
    }
}

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_WRITE_NZ(io, "shade", m_Shade);
    JSONW_WRITE_NZ(io, "start_idx", m_StartIndex);
    JSONW_WRITE(io, "owner", m_OwnerNum);

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_POINTS; i++) {
        for (int32_t side = 0; side < 2; side++) {
            const M_POINT *const pt = &m_Points[i][side];
            if (pt->life == 0) {
                continue;
            }
            JSONW_PUSH_OBJECT(io);
            JSONW_WRITE(io, "idx", i);
            JSONW_WRITE(io, "side", side);
            JSONW_WRITE(io, "life", pt->life);
            JSONW_WRITE(io, "pos_outer", pt->pos[0]);
            JSONW_WRITE(io, "pos_inner", pt->pos[1]);
            JSONW_POP_AND_APPEND(io);
        }
    }
    JSONW_POP_AND_SET_NZ(io, "points");
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    MUST(JSON_READ_D(io, "shade", &m_Shade, 0));
    MUST(JSON_READ_D(io, "start_idx", &m_StartIndex, 0));
    if (m_StartIndex >= M_MAX_POINTS) {
        return JSON_ReadIO_Fail(
            io, "wake start index %d is out of range", m_StartIndex);
    }
    MUST(JSON_READ_D(io, "owner", &m_OwnerNum, NO_ITEM));

    if (!JSON_ReadIO_HasKey(io, "points")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "points"));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        MUST(JSON_PUSH_INDEX(io, i));
        int32_t idx = 0;
        int32_t side = 0;
        MUST(JSON_READ(io, "idx", &idx));
        MUST(JSON_READ(io, "side", &side));
        if (idx < 0 || idx >= M_MAX_POINTS || side < 0 || side > 1) {
            return JSON_ReadIO_Fail(
                io, "wake point %d/%d is not there", idx, side);
        }
        M_POINT *const pt = &m_Points[idx][side];
        MUST(JSON_READ(io, "life", &pt->life));
        MUST(JSON_READ(io, "pos_outer", &pt->pos[0]));
        MUST(JSON_READ(io, "pos_inner", &pt->pos[1]));
        MUST(JSON_POP(io));
    }

    MUST(JSON_POP(io));
    return OK;
}

void FX_UPVWake_Reset(void)
{
    for (int32_t i = 0; i < M_MAX_POINTS; i++) {
        m_Points[i][0].life = 0;
        m_Points[i][1].life = 0;
    }
    m_Shade = 0;
    m_StartIndex = 0;
    m_OwnerNum = NO_ITEM;
}

void FX_UPVWake_Spawn(
    const ITEM *const item, const int32_t side, const XYZ_32 pos[2])
{
    m_OwnerNum = Item_GetIndex(item);
    M_POINT *const pt = &m_Points[m_StartIndex][side];
    if (pt->life != 0) {
        return;
    }

    pt->life = M_MAX_LIFE;
    pt->pos[0] = pos[0];
    pt->pos[1] = pos[1];

    if (side == 1) {
        m_StartIndex = (m_StartIndex + 1) & (M_MAX_POINTS - 1);
    }
}

void FX_UPVWake_UpdateShade(const ITEM *const item, const bool is_moving)
{
    if (!M_IsOwner(item)) {
        return;
    }

    if (!is_moving) {
        if (m_Shade > 0) {
            m_Shade--;
        }
    } else if (m_Shade < M_MAX_SHADE) {
        m_Shade += 2;
    }
}

void FX_UPVWake_Draw(
    const ITEM *const item, const int32_t side, const XYZ_32 origin[2])
{
    if (!M_IsOwner(item)) {
        return;
    }

    XYZ_32 prev[2] = { origin[0], origin[1] };
    const RGBA_8888 edge_color = { 0, 0, 0, 255 };
    RGBA_8888 prev_color = edge_color;

    int32_t current = (m_StartIndex - 1) & (M_MAX_POINTS - 1);
    for (int32_t i = 0; i < M_MAX_POINTS; i++) {
        const M_POINT *const pt = &m_Points[current][side];
        if (pt->life == 0) {
            break;
        }

        const RGBA_8888 color = M_GetColor(pt->life >> 2);
        const XYZ_32 quad[4] = { prev[1], prev[0], pt->pos[0], pt->pos[1] };
        const RGBA_8888 quad_color[4] = {
            edge_color,
            prev_color,
            color,
            edge_color,
        };
        OutputSource_PolyFX_StageQuadExt(
            -1, quad, nullptr, quad_color,
            VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE,
            DRAW_BLEND_ADD);

        prev[0] = pt->pos[0];
        prev[1] = pt->pos[1];
        prev_color = color;
        current = (current - 1) & (M_MAX_POINTS - 1);
    }
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .reset_func = FX_UPVWake_Reset,
    .save_key = "upv_wake",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
