#include <trx/game/fx/bubble.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/fx/common.h>
#include <trx/game/fx/water.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/output/const.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/state.h>
#include <trx/game/output/vars.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks/manager.h>
#include <trx/version.h>

#include <string.h>

#define M_MAX_BUBBLES 40
#define M_MAX_SHADE 144
#define M_DRAW_RANGE 0x5000
#define M_MIN_DRAW_DEPTH 32
#define M_MIN_SIZE 4
#define M_MAX_SIZE 128

typedef struct {
    XYZ_32 pos;
    XYZ_32 prev_pos;
    int16_t room_num;
    int16_t speed;
    int16_t size;
    int16_t dsize;
    uint8_t shade;
    uint8_t vel;
    int16_t pad;
} M_BUBBLE;

static M_BUBBLE m_Bubbles[M_MAX_BUBBLES];
static int32_t m_NextBubble = 0;

static M_BUBBLE *M_GetFree(void)
{
    int32_t idx = m_NextBubble;
    for (int32_t i = 0; i < M_MAX_BUBBLES; i++) {
        if (m_Bubbles[idx].size == 0) {
            break;
        }
        idx = (idx + 1) % M_MAX_BUBBLES;
    }
    m_NextBubble = (idx + 1) % M_MAX_BUBBLES;
    return &m_Bubbles[idx];
}

static int64_t M_GetViewDepth(const XYZ_32 pos)
{
    // clang-format off
    return
        g_ViewMatrix._20 * pos.x +
        g_ViewMatrix._21 * pos.y +
        g_ViewMatrix._22 * pos.z +
        g_ViewMatrix._23;
    // clang-format on
}

static void M_Reset(void)
{
    memset(m_Bubbles, 0, sizeof(m_Bubbles));
    m_NextBubble = 0;
}

static void M_Control(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    for (int32_t i = 0; i < M_MAX_BUBBLES; i++) {
        M_BUBBLE *const bubble = &m_Bubbles[i];
        if (bubble->size == 0) {
            continue;
        }

        bubble->prev_pos = bubble->pos;
        bubble->pad += 6;
        bubble->speed += bubble->vel;
        bubble->pos.x += (3 * Math_Sin(bubble->pad << 8)) >> W2V_SHIFT;
        bubble->pos.y -= bubble->speed >> 8;
        bubble->pos.z += Math_Cos(bubble->pad << 8) >> W2V_SHIFT;

        if (lara_item != nullptr) {
            const int32_t dx = bubble->pos.x - lara_item->pos.x;
            const int32_t dy = bubble->pos.y - lara_item->pos.y;
            const int32_t dz = bubble->pos.z - lara_item->pos.z;
            if (ABS(dx) > M_DRAW_RANGE || ABS(dy) > M_DRAW_RANGE
                || ABS(dz) > M_DRAW_RANGE) {
                bubble->size = 0;
                continue;
            }
        }

        int16_t room_num = bubble->room_num;
        const SECTOR *const sector = Room_GetSector(bubble->pos, &room_num);
        if (sector == nullptr
            || bubble->pos.y > Room_GetHeight(sector, bubble->pos)) {
            bubble->size = 0;
            continue;
        }

        if (!Room_Get(room_num)->flags.underwater) {
            const XYZ_32 ripple_pos = {
                .x = bubble->pos.x,
                .y = Room_Get(bubble->room_num)->max_ceiling,
                .z = bubble->pos.z,
            };
            FX_Water_SetupRipple(
                ripple_pos, (Random_GetControl() & 0xF) + 48, FX_RIPPLE_SLOW);
            bubble->size = 0;
            continue;
        }

        const int32_t ceiling = Room_GetCeiling(sector, bubble->pos);
        if (ceiling == NO_HEIGHT || bubble->pos.y <= ceiling) {
            bubble->size = 0;
            continue;
        }

        if (bubble->size < bubble->dsize) {
            bubble->size++;
        }
        if (bubble->shade < M_MAX_SHADE) {
            bubble->shade += 2;
        }
        bubble->room_num = room_num;
    }
}

static void M_Draw(void)
{
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_BUBBLE);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    const double ratio = Interpolation_GetWorldRate();
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;

    for (int32_t i = 0; i < M_MAX_BUBBLES; i++) {
        const M_BUBBLE *const bubble = &m_Bubbles[i];
        if (bubble->size == 0) {
            continue;
        }

        const XYZ_32 center = do_interp
            ? (XYZ_32) {
                  .x = (int32_t)LERP(bubble->prev_pos.x, bubble->pos.x, ratio),
                  .y = (int32_t)LERP(bubble->prev_pos.y, bubble->pos.y, ratio),
                  .z = (int32_t)LERP(bubble->prev_pos.z, bubble->pos.z, ratio),
              }
            : bubble->pos;

        const int32_t vpos_z = (int32_t)(M_GetViewDepth(center) >> W2V_SHIFT);
        if (vpos_z < M_MIN_DRAW_DEPTH || vpos_z > M_DRAW_RANGE) {
            continue;
        }

        // The original clamps the bubble in the screen pixels of its
        // reference viewport, so the clamp is applied there and the result
        // taken back into world units.
        int32_t size = REF_PERSP * (bubble->size >> 1) / vpos_z;
        if (size > M_MAX_SIZE) {
            continue;
        }
        CLAMPL(size, M_MIN_SIZE);
        const float half = (float)((size >> 1) * vpos_z) / REF_PERSP;

        const RGBA_8888 color = { bubble->shade, bubble->shade, bubble->shade,
                                  255 };
        const RGBA_8888 colors[4] = { color, color, color, color };
        const XYZ_32 world_pos[4] = { center, center, center, center };
        const float disp[4][2] = {
            { -half, -half },
            { half, -half },
            { half, half },
            { -half, half },
        };

        OutputSource_PolyFX_StageQuadExt(
            sprite_idx, world_pos, disp, colors,
            VERT_NO_LIGHTING | VERT_NO_WIBBLE | VERT_BILLBOARD
                | VERT_ABS_SPRITE,
            DRAW_BLEND_ADD);
    }
}

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_BUBBLES; i++) {
        const M_BUBBLE *const bubble = &m_Bubbles[i];
        if (bubble->size == 0) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", bubble->pos);
        JSONW_WRITE(io, "room_num", bubble->room_num);
        JSONW_WRITE(io, "speed", bubble->speed);
        JSONW_WRITE(io, "size", bubble->size);
        JSONW_WRITE(io, "dsize", bubble->dsize);
        JSONW_WRITE(io, "shade", bubble->shade);
        JSONW_WRITE(io, "vel", bubble->vel);
        JSONW_WRITE(io, "pad", bubble->pad);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "bubbles");
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "bubbles")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "bubbles"));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= M_MAX_BUBBLES) {
            LOG_WARNING(
                "Malformed save: too many bubbles. Extra bubbles will be "
                "ignored.");
            break;
        }

        M_BUBBLE *const bubble = &m_Bubbles[i];
        MUST(JSON_PUSH_INDEX(io, i));
        MUST(JSON_READ(io, "pos", &bubble->pos));
        MUST(JSON_READ(io, "room_num", &bubble->room_num));
        MUST(JSON_READ(io, "speed", &bubble->speed));
        MUST(JSON_READ(io, "size", &bubble->size));
        MUST(JSON_READ(io, "dsize", &bubble->dsize));
        MUST(JSON_READ(io, "shade", &bubble->shade));
        MUST(JSON_READ(io, "vel", &bubble->vel));
        MUST(JSON_READ(io, "pad", &bubble->pad));
        MUST(JSON_POP(io));
        bubble->prev_pos = bubble->pos;
        m_NextBubble = (i + 1) % M_MAX_BUBBLES;
    }

    MUST(JSON_POP(io));
    return OK;
}

void FX_Bubble_Create(
    const XYZ_32 pos, int16_t room_num, const int32_t size,
    const int32_t biggest)
{
    Room_GetSector(pos, &room_num);
    if (!Room_Get(room_num)->flags.underwater) {
        return;
    }

    M_BUBBLE *const bubble = M_GetFree();
    bubble->pos = pos;
    bubble->prev_pos = pos;
    bubble->room_num = room_num;
    bubble->speed = (int16_t)((Random_GetControl() & 0xFF) + 64);
    bubble->shade = 0;
    bubble->size = (int16_t)((size + (biggest & Random_GetControl())) << 1);
    bubble->dsize = (int16_t)(bubble->size << 4);
    bubble->vel = (uint8_t)((Random_GetControl() & 0x1F) + 32);
    bubble->pad = 0;
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = M_Draw,
    .reset_func = M_Reset,
    .save_key = "bubbles",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
