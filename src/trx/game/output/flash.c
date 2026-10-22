#include <trx/game/output/flash.h>

#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/output/vars.h>
#include <trx/game/viewport.h>

// clang-format off
#define M_FADER_MAX  32
#define M_FADER_STEP 2
// How far in front of the camera the flash is drawn, in world units.
#define M_VIEW_DIST  8192
// clang-format on

static int32_t m_Fader = 0;
static RGB_888 m_Color = {};

void Output_Flash_Trigger(const RGB_888 color)
{
    m_Fader = M_FADER_MAX;
    m_Color = color;
}

void Output_Flash_Reset(void)
{
    m_Fader = 0;
    m_Color = (RGB_888) {};
}

void Output_Flash_Update(void)
{
    if (m_Fader > 0) {
        m_Fader -= M_FADER_STEP;
        CLAMPL(m_Fader, 0);
    }
}

void Output_Flash_Draw(void)
{
    if (m_Fader <= 0) {
        return;
    }
    const RGBA_8888 color = {
        .r = m_Color.r * m_Fader / M_FADER_MAX,
        .g = m_Color.g * m_Fader / M_FADER_MAX,
        .b = m_Color.b * m_Fader / M_FADER_MAX,
        .a = 255,
    };
    // A single flat additive billboard covering the whole screen.
    const float half_w = (float)((double)Viewport_GetWidth(VIEWPORT_GAME) / 2.0
                                 * M_VIEW_DIST / g_PhdPersp);
    const float half_h = (float)((double)Viewport_GetHeight(VIEWPORT_GAME) / 2.0
                                 * M_VIEW_DIST / g_PhdPersp);

    const MATRIX *const m = &g_ViewMatrix;
    const double w2v = (double)(1 << W2V_SHIFT);
    const XYZ_32 anchor = {
        .x = g_ViewPos.x + (int32_t)(m->_20 * M_VIEW_DIST / w2v),
        .y = g_ViewPos.y + (int32_t)(m->_21 * M_VIEW_DIST / w2v),
        .z = g_ViewPos.z + (int32_t)(m->_22 * M_VIEW_DIST / w2v),
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
        -(float)M_VIEW_DIST, DRAW_BLEND_ADD);
}
