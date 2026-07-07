#include <trx/game/lara/poison.h>

#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara/common.h>
#include <trx/game/output.h>
#include <trx/version.h>

// TR4 poison vision: the viewport is warped by scaling the view matrix
// per-axis with two out-of-phase sine oscillators per axis.
#define M_EFFECT_THRESHOLD 256
#define M_MAX_POISON 4096
#define M_ONE (1 << W2V_SHIFT)

static const XYZ_16 m_PhaseStepA = { .x = 150, .y = 230, .z = 660 };
static const XYZ_16 m_PhaseStepB = { .x = 270, .y = 440, .z = 160 };
static XYZ_16 m_PhaseA = {};
static XYZ_16 m_PhaseB = {};

static int32_t M_AxisScale(
    int32_t value, int16_t phase_a, int16_t phase_step_a, int16_t phase_b,
    int16_t phase_step_b, double lerp);

static int32_t M_AxisScale(
    const int32_t value, const int16_t phase_a, const int16_t phase_step_a,
    const int16_t phase_b, const int16_t phase_step_b, const double lerp)
{
    // lerp between the previous and the current tick's phase
    const int16_t off_a =
        phase_a - phase_step_a + (int16_t)(phase_step_a * lerp);
    const int16_t off_b =
        phase_b - phase_step_b + (int16_t)(phase_step_b * lerp);
    const int32_t wave = (Math_Sin(off_a) + Math_Sin(off_b)) >> 2;
    return (((value - M_EFFECT_THRESHOLD) * wave) >> 12) + M_ONE;
}

void Lara_Poison_Tick(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const int32_t time4 = (int32_t)Output_GetTimeInGame() * 4;

    if (g_TRVersion != 4) {
        if (lara_info->poison.value >= 16 && (time4 & 0xFF) == 0) {
            CLAMPG(lara_info->poison.value, 256);
            Lara_TakeDamage(lara_info->poison.value >> 4, false);
        }
        return;
    }

    m_PhaseA.x += m_PhaseStepA.x;
    m_PhaseA.y += m_PhaseStepA.y;
    m_PhaseA.z += m_PhaseStepA.z;
    m_PhaseB.x += m_PhaseStepB.x;
    m_PhaseB.y += m_PhaseStepB.y;
    m_PhaseB.z += m_PhaseStepB.z;

    if (lara_info->poison.value != lara_info->poison.target) {
        lara_info->poison.value +=
            (lara_info->poison.target - lara_info->poison.value) >> 4;
        if (ABS(lara_info->poison.target - lara_info->poison.value) < 16) {
            lara_info->poison.value = lara_info->poison.target;
        }
    }

    if (lara_info->poison.value != 0) {
        if (lara_info->poison.value > M_MAX_POISON) {
            lara_info->poison.value = M_MAX_POISON;
        } else if (lara_info->poison.target != 0) {
            lara_info->poison.target++;
        }
        if (lara_info->poison.value >= M_EFFECT_THRESHOLD
            && (time4 & 0xFF) == 0) {
            Lara_TakeDamage(lara_info->poison.value >> 8, false);
        }
    }
}

void Lara_Poison_Reset(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    lara_info->poison.value = 0;
    lara_info->poison.target = 0;
}

void Lara_Poison_Cure(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    // TR4 medipacks only clear the target, so the vision effect eases out
    // via the interpolation instead of snapping
    lara_info->poison.target = 0;
    if (g_TRVersion != 4) {
        lara_info->poison.value = 0;
    }
}

bool Lara_Poison_GetViewScale(XYZ_32 *const scale)
{
    if (g_TRVersion != 4 || g_Camera.type == CAM_PHOTO_MODE) {
        return false;
    }
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const int32_t value = lara_info->poison.value;
    if (value < M_EFFECT_THRESHOLD) {
        return false;
    }

    const double lerp = Interpolation_GetRate();
    scale->x = M_AxisScale(
        value, m_PhaseA.x, m_PhaseStepA.x, m_PhaseB.x, m_PhaseStepB.x, lerp);
    scale->y = M_AxisScale(
        value, m_PhaseA.y, m_PhaseStepA.y, m_PhaseB.y, m_PhaseStepB.y, lerp);
    scale->z = M_AxisScale(
        value, m_PhaseA.z, m_PhaseStepA.z, m_PhaseB.z, m_PhaseStepB.z, lerp);
    return scale->x != M_ONE || scale->y != M_ONE || scale->z != M_ONE;
}
