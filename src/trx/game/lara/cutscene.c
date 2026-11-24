#include <trx/game/lara/cutscene.h>

#include <trx/config.h>
#include <trx/game/collision.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/misc.h>
#include <trx/game/gun/smoke.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/random.h>
#include <trx/version.h>

typedef struct {
    bool is_valid;
    LARA_SKIN_TYPE skin_type;
    LARA_GUN_TYPE hand_l_type;
    LARA_GUN_TYPE hand_r_type;
    LARA_GUN_TYPE thigh_l_type;
    LARA_GUN_TYPE thigh_r_type;
    bool holsters_visible;
} M_LARA_CUTSCENE_STATE;

static bool m_DrawLeftGunFlash = false;
static bool m_DrawRightGunFlash = false;
static M_LARA_CUTSCENE_STATE m_LaraCutsceneState = {};

static LARA_GUN_TYPE M_GetGunEquipmentType(const LARA_MESH mesh)
{
    const LARA_SKIN_EQUIPMENT *const equipment = Lara_Skin_GetEquipment(mesh);
    if (equipment->type == EQUIPMENT_TYPE_WEAPON) {
        return (LARA_GUN_TYPE)equipment->data;
    }
    return LGT_UNARMED;
}

static void M_DrawGunFlash(const LARA_MESH hand_mesh)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    XYZ_32 pos = {};
    const bool has_mesh_pos = Lara_GetMeshPos(hand_mesh, &pos);
    if (!has_mesh_pos) {
        const ITEM *const lara_item = Lara_GetItem();
        Collide_GetJointAbsPosition(lara_item, &pos, hand_mesh);
    }

    Matrix_Push();
    *g_MatrixPtr = g_ViewMatrix;
    *g_WMatrixPtr = g_IDMatrix;
    Matrix_TranslateAbs32(pos);
    if (has_mesh_pos && lara->mesh_pos_matrices_valid) {
        MATRIX hand_rot = lara->mesh_pos_matrices[hand_mesh];
        hand_rot._03 = 0;
        hand_rot._13 = 0;
        hand_rot._23 = 0;
        Matrix_Mul3x3(&hand_rot);
    }
    Gun_DrawFlash(Gun_GetDefaultType(), CLIP_FULLY_VISIBLE, false);
    Matrix_Pop();
}

void Lara_Cutscene_CaptureState(void)
{
    m_LaraCutsceneState.is_valid = true;
    m_LaraCutsceneState.skin_type = Lara_Skin_GetType();
    m_LaraCutsceneState.hand_l_type = M_GetGunEquipmentType(LM_HAND_L);
    m_LaraCutsceneState.hand_r_type = M_GetGunEquipmentType(LM_HAND_R);
    m_LaraCutsceneState.thigh_l_type = M_GetGunEquipmentType(LM_THIGH_L);
    m_LaraCutsceneState.thigh_r_type = M_GetGunEquipmentType(LM_THIGH_R);
    m_LaraCutsceneState.holsters_visible = Lara_Skin_AreHolstersVisible();
}

void Lara_Cutscene_RestoreState(void)
{
    if (!m_LaraCutsceneState.is_valid) {
        return;
    }

    Lara_Skin_SetType(m_LaraCutsceneState.skin_type);
    Lara_Skin_SetGunEquipment(LM_HAND_L, m_LaraCutsceneState.hand_l_type);
    Lara_Skin_SetGunEquipment(LM_HAND_R, m_LaraCutsceneState.hand_r_type);
    Lara_Skin_SetGunEquipment(LM_THIGH_L, m_LaraCutsceneState.thigh_l_type);
    Lara_Skin_SetGunEquipment(LM_THIGH_R, m_LaraCutsceneState.thigh_r_type);
    Lara_Skin_SetHolstersVisible(m_LaraCutsceneState.holsters_visible);
}

void Lara_Cutscene_ControlGun(void)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const ITEM *const lara_item = Lara_GetItem();

    m_DrawLeftGunFlash = false;
    m_DrawRightGunFlash = false;
    if (lara->left_arm.flash_gun > 0) {
        m_DrawLeftGunFlash = true;
        lara->left_arm.flash_gun--;
    }
    if (lara->right_arm.flash_gun > 0) {
        m_DrawRightGunFlash = true;
        lara->right_arm.flash_gun--;
    }

    Gun_Smoke_Control();

    if (g_Config.visuals.enable_gun_lighting
        && (m_DrawLeftGunFlash || m_DrawRightGunFlash)) {
        XYZ_32 pos = { .x = -12, .y = 48, .z = 40 };
        LARA_MESH mesh = LM_HAND_L;
        if (m_DrawRightGunFlash) {
            pos.x = 8;
            mesh = LM_HAND_R;
        }

        Collide_GetJointAbsPosition(lara_item, &pos, mesh);
        pos.x += (Random_GetControl() & 0xFF) - 128;
        pos.y -= (Random_GetControl() & 0x7F) - 63;
        pos.z += (Random_GetControl() & 0xFF) - 128;
        if (g_TRVersion >= 3) {
            const RGB_888 color = {
                .r = 192 + (Random_GetControl() & 0x3F),
                .g = 144 + (Random_GetControl() & 0x1F),
                .b = Random_GetControl() & 0x3F,
            };
            Output_AddDynamicLightRGB(pos, 10, color);
        } else {
            Output_AddDynamicLight(pos, 10, 11);
        }
    }
}

void Lara_Cutscene_ResetGunFlashes(void)
{
    m_DrawLeftGunFlash = false;
    m_DrawRightGunFlash = false;
}

void Lara_Cutscene_DrawGunFlashes(void)
{
    if (m_DrawLeftGunFlash) {
        M_DrawGunFlash(LM_HAND_L);
    }
    if (m_DrawRightGunFlash) {
        M_DrawGunFlash(LM_HAND_R);
    }
}
