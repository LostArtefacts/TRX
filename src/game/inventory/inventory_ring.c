#include "game/inventory/inventory_ring.h"

#include "game/gameflow.h"
#include "game/inventory/inventory_func.h"
#include "game/inventory/inventory_vars.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math_misc.h"

#include <stdint.h>
#include <stdio.h>

TEXTSTRING *g_InvItemText[IT_NUMBER_OF] = { NULL };
TEXTSTRING *g_InvRingText = NULL;

static TEXTSTRING *m_InvDownArrow1 = NULL;
static TEXTSTRING *m_InvDownArrow2 = NULL;
static TEXTSTRING *m_InvUpArrow1 = NULL;
static TEXTSTRING *m_InvUpArrow2 = NULL;

void Inv_Ring_IsOpen(RING_INFO *ring)
{
    if (g_InvMode == INV_TITLE_MODE) {
        return;
    }

    if (!g_InvRingText) {
        switch (ring->type) {
        case RT_MAIN:
            g_InvRingText =
                Text_Create(0, 26, g_GameFlow.strings[GS_HEADING_INVENTORY]);
            break;

        case RT_OPTION:
            if (g_InvMode == INV_DEATH_MODE) {
                g_InvRingText = Text_Create(
                    0, 26, g_GameFlow.strings[GS_HEADING_GAME_OVER]);
            } else {
                g_InvRingText =
                    Text_Create(0, 26, g_GameFlow.strings[GS_HEADING_OPTION]);
            }
            break;

        case RT_KEYS:
            g_InvRingText =
                Text_Create(0, 26, g_GameFlow.strings[GS_HEADING_ITEMS]);
            break;
        }

        Text_CentreH(g_InvRingText, 1);
    }

    if (g_InvMode != INV_GAME_MODE) {
        return;
    }

    if (!m_InvUpArrow1) {
        if (ring->type == RT_OPTION
            || (ring->type == RT_MAIN && g_InvKeysObjects)) {
            m_InvUpArrow1 = Text_Create(20, 28, "[");
            m_InvUpArrow2 = Text_Create(-20, 28, "[");
            Text_AlignRight(m_InvUpArrow2, 1);
        }
    }

    if (!m_InvDownArrow1) {
        if (ring->type == RT_MAIN || ring->type == RT_KEYS) {
            m_InvDownArrow1 = Text_Create(20, -15, "]");
            m_InvDownArrow2 = Text_Create(-20, -15, "]");
            Text_AlignBottom(m_InvDownArrow1, 1);
            Text_AlignBottom(m_InvDownArrow2, 1);
            Text_AlignRight(m_InvDownArrow2, 1);
        }
    }
}

void RingIsNotOpen(RING_INFO *ring)
{
    if (!g_InvRingText) {
        return;
    }

    Text_Remove(g_InvRingText);
    g_InvRingText = NULL;

    if (m_InvUpArrow1) {
        Text_Remove(m_InvUpArrow1);
        Text_Remove(m_InvUpArrow2);
        m_InvUpArrow1 = NULL;
        m_InvUpArrow2 = NULL;
    }
    if (m_InvDownArrow1) {
        Text_Remove(m_InvDownArrow1);
        Text_Remove(m_InvDownArrow2);
        m_InvDownArrow1 = NULL;
        m_InvDownArrow2 = NULL;
    }
}

void RingActive(INVENTORY_ITEM *inv_item)
{
    if (!g_InvItemText[IT_NAME]) {
        switch (inv_item->object_number) {
        case O_PUZZLE_OPTION1:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].puzzle1);
            break;

        case O_PUZZLE_OPTION2:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].puzzle2);
            break;

        case O_PUZZLE_OPTION3:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].puzzle3);
            break;

        case O_PUZZLE_OPTION4:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].puzzle4);
            break;

        case O_KEY_OPTION1:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].key1);
            break;

        case O_KEY_OPTION2:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].key2);
            break;

        case O_KEY_OPTION3:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].key3);
            break;

        case O_KEY_OPTION4:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].key4);
            break;

        case O_PICKUP_OPTION1:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].pickup1);
            break;

        case O_PICKUP_OPTION2:
            g_InvItemText[IT_NAME] =
                Text_Create(0, -16, g_GameFlow.levels[g_CurrentLevel].pickup2);
            break;

        case O_PASSPORT_OPTION:
            break;

        default:
            g_InvItemText[IT_NAME] = Text_Create(0, -16, inv_item->string);
            break;
        }

        if (g_InvItemText[IT_NAME]) {
            Text_AlignBottom(g_InvItemText[IT_NAME], 1);
            Text_CentreH(g_InvItemText[IT_NAME], 1);
        }
    }

    char temp_text[64];
    int32_t qty = Inv_RequestItem(inv_item->object_number);

    switch (inv_item->object_number) {
    case O_SHOTGUN_OPTION:
        if (!g_InvItemText[IT_QTY] && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
            sprintf(
                temp_text, "%5d A", g_Lara.shotgun.ammo / SHOTGUN_AMMO_CLIP);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_MAGNUM_OPTION:
        if (!g_InvItemText[IT_QTY] && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
            sprintf(temp_text, "%5d B", g_Lara.magnums.ammo);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_UZI_OPTION:
        if (!g_InvItemText[IT_QTY] && !(g_GameInfo.bonus_flag & GBF_NGPLUS)) {
            sprintf(temp_text, "%5d C", g_Lara.uzis.ammo);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_SG_AMMO_OPTION:
        if (!g_InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", qty * NUM_SG_SHELLS);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_MAG_AMMO_OPTION:
        if (!g_InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", Inv_RequestItem(O_MAG_AMMO_OPTION) * 2);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_UZI_AMMO_OPTION:
        if (!g_InvItemText[IT_QTY]) {
            sprintf(temp_text, "%d", Inv_RequestItem(O_UZI_AMMO_OPTION) * 2);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_MEDI_OPTION:
        Overlay_BarSetHealthTimer(40);
        Overlay_BarDrawHealth();
        if (!g_InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_BIGMEDI_OPTION:
        Overlay_BarSetHealthTimer(40);
        Overlay_BarDrawHealth();
        if (!g_InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    case O_KEY_OPTION1:
    case O_KEY_OPTION2:
    case O_KEY_OPTION3:
    case O_KEY_OPTION4:
    case O_LEADBAR_OPTION:
    case O_PICKUP_OPTION1:
    case O_PICKUP_OPTION2:
    case O_PUZZLE_OPTION1:
    case O_PUZZLE_OPTION2:
    case O_PUZZLE_OPTION3:
    case O_PUZZLE_OPTION4:
    case O_SCION_OPTION:
        if (!g_InvItemText[IT_QTY] && qty > 1) {
            sprintf(temp_text, "%d", qty);
            Overlay_MakeAmmoString(temp_text);
            g_InvItemText[IT_QTY] = Text_Create(64, -56, temp_text);
            Text_AlignBottom(g_InvItemText[IT_QTY], 1);
            Text_CentreH(g_InvItemText[IT_QTY], 1);
        }
        break;

    default:
        break;
    }
}

void RingNotActive(void)
{
    RemoveInventoryText();
}

void RemoveInventoryText(void)
{
    for (int i = 0; i < IT_NUMBER_OF; i++) {
        if (g_InvItemText[i]) {
            Text_Remove(g_InvItemText[i]);
            g_InvItemText[i] = NULL;
        }
    }
}

void Inv_Ring_Init(
    RING_INFO *ring, int16_t type, INVENTORY_ITEM **list, int16_t qty,
    int16_t current, IMOTION_INFO *imo)
{
    ring->type = type;
    ring->radius = 0;
    ring->list = list;
    ring->number_of_objects = qty;
    ring->current_object = current;
    ring->angle_adder = 0x10000 / qty;

    if (g_InvMode == INV_TITLE_MODE) {
        ring->camera_pitch = 1024;
    } else {
        ring->camera_pitch = 0;
    }
    ring->rotating = 0;
    ring->rot_count = 0;
    ring->target_object = 0;
    ring->rot_adder = 0;
    ring->rot_adder_l = 0;
    ring->rot_adder_r = 0;

    ring->imo = imo;

    ring->camera.x = 0;
    ring->camera.y = CAMERA_STARTHEIGHT;
    ring->camera.z = 896;
    ring->camera.x_rot = 0;
    ring->camera.y_rot = 0;
    ring->camera.z_rot = 0;

    Inv_Ring_MotionInit(ring, OPEN_FRAMES, RNG_OPENING, RNG_OPEN);
    Inv_Ring_MotionRadius(ring, RING_RADIUS);
    Inv_Ring_MotionCameraPos(ring, CAMERA_HEIGHT);
    Inv_Ring_MotionRotation(
        ring, OPEN_ROTATION,
        0xC000 - (ring->current_object * ring->angle_adder));

    ring->ringpos.x = 0;
    ring->ringpos.y = 0;
    ring->ringpos.z = 0;
    ring->ringpos.x_rot = 0;
    ring->ringpos.y_rot = imo->rotate_target - OPEN_ROTATION;
    ring->ringpos.z_rot = 0;

    ring->light.x = -1536;
    ring->light.y = 256;
    ring->light.z = 1024;
}

void Inv_Ring_GetView(RING_INFO *ring, PHD_3DPOS *viewer)
{
    PHD_ANGLE angles[2];

    Math_GetVectorAngles(
        -ring->camera.x, CAMERA_YOFFSET - ring->camera.y,
        ring->radius - ring->camera.z, angles);
    viewer->x = ring->camera.x;
    viewer->y = ring->camera.y;
    viewer->z = ring->camera.z;
    viewer->x_rot = angles[1] + ring->camera_pitch;
    viewer->y_rot = angles[0];
    viewer->z_rot = 0;
}

void Inv_Ring_Light(RING_INFO *ring)
{
    PHD_ANGLE angles[2];
    g_LsDivider = 0x6000;
    Math_GetVectorAngles(ring->light.x, ring->light.y, ring->light.z, angles);
    Output_RotateLight(angles[1], angles[0]);
}

void Inv_Ring_CalcAdders(RING_INFO *ring, int16_t rotation_duration)
{
    ring->angle_adder = 0x10000 / ring->number_of_objects;
    ring->rot_adder_l = ring->angle_adder / rotation_duration;
    ring->rot_adder_r = -ring->rot_adder_l;
}

void Inv_Ring_DoMotions(RING_INFO *ring)
{
    IMOTION_INFO *imo = ring->imo;

    if (imo->count) {
        ring->radius += imo->radius_rate;
        ring->camera.y += imo->camera_yrate;
        ring->ringpos.y_rot += imo->rotate_rate;
        ring->camera_pitch += imo->camera_pitch_rate;

        INVENTORY_ITEM *inv_item = ring->list[ring->current_object];
        inv_item->pt_xrot += imo->item_ptxrot_rate;
        inv_item->x_rot += imo->item_xrot_rate;
        inv_item->ytrans += imo->item_ytrans_rate;
        inv_item->ztrans += imo->item_ztrans_rate;

        imo->count--;
        if (!imo->count) {
            imo->status = imo->status_target;
            if (imo->radius_rate) {
                imo->radius_rate = 0;
                ring->radius = imo->radius_target;
            }
            if (imo->camera_yrate) {
                imo->camera_yrate = 0;
                ring->camera.y = imo->camera_ytarget;
            }
            if (imo->rotate_rate) {
                imo->rotate_rate = 0;
                ring->ringpos.y_rot = imo->rotate_target;
            }
            if (imo->item_ptxrot_rate) {
                imo->item_ptxrot_rate = 0;
                inv_item->pt_xrot = imo->item_ptxrot_target;
            }
            if (imo->item_xrot_rate) {
                imo->item_xrot_rate = 0;
                inv_item->x_rot = imo->item_xrot_target;
            }
            if (imo->item_ytrans_rate) {
                imo->item_ytrans_rate = 0;
                inv_item->ytrans = imo->item_ytrans_target;
            }
            if (imo->item_ztrans_rate) {
                imo->item_ztrans_rate = 0;
                inv_item->ztrans = imo->item_ztrans_target;
            }
            if (imo->camera_pitch_rate) {
                imo->camera_pitch_rate = 0;
                ring->camera_pitch = imo->camera_pitch_target;
            }
        }
    }

    if (ring->rotating) {
        ring->ringpos.y_rot += ring->rot_adder;
        ring->rot_count--;
        if (!ring->rot_count) {
            ring->current_object = ring->target_object;
            ring->ringpos.y_rot =
                0xC000 - (ring->current_object * ring->angle_adder);
            ring->rotating = 0;
        }
    }
}

void Inv_Ring_RotateLeft(RING_INFO *ring)
{
    ring->rotating = 1;
    ring->target_object = ring->current_object - 1;
    if (ring->target_object < 0) {
        ring->target_object = ring->number_of_objects - 1;
    }
    ring->rot_count = ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_l;
}

void Inv_Ring_RotateRight(RING_INFO *ring)
{
    ring->rotating = 1;
    ring->target_object = ring->current_object + 1;
    if (ring->target_object >= ring->number_of_objects) {
        ring->target_object = 0;
    }
    ring->rot_count = ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_r;
}

void Inv_Ring_MotionInit(
    RING_INFO *ring, int16_t frames, int16_t status, int16_t status_target)
{
    ring->imo->status_target = status_target;
    ring->imo->count = frames;
    ring->imo->status = status;
    ring->imo->radius_target = 0;
    ring->imo->radius_rate = 0;
    ring->imo->camera_ytarget = 0;
    ring->imo->camera_yrate = 0;
    ring->imo->camera_pitch_target = 0;
    ring->imo->camera_pitch_rate = 0;
    ring->imo->rotate_target = 0;
    ring->imo->rotate_rate = 0;
    ring->imo->item_ptxrot_target = 0;
    ring->imo->item_ptxrot_rate = 0;
    ring->imo->item_xrot_target = 0;
    ring->imo->item_xrot_rate = 0;
    ring->imo->item_ytrans_target = 0;
    ring->imo->item_ytrans_rate = 0;
    ring->imo->item_ztrans_target = 0;
    ring->imo->item_ztrans_rate = 0;
    ring->imo->misc = 0;
}

void Inv_Ring_MotionSetup(
    RING_INFO *ring, int16_t status, int16_t status_target, int16_t frames)
{
    IMOTION_INFO *imo = ring->imo;
    imo->count = frames;
    imo->status = status;
    imo->status_target = status_target;
    imo->radius_rate = 0;
    imo->camera_yrate = 0;
}

void Inv_Ring_MotionRadius(RING_INFO *ring, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->radius_target = target;
    imo->radius_rate = (target - ring->radius) / imo->count;
}

void Inv_Ring_MotionRotation(RING_INFO *ring, int16_t rotation, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->rotate_target = target;
    imo->rotate_rate = rotation / imo->count;
}

void Inv_Ring_MotionCameraPos(RING_INFO *ring, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->camera_ytarget = target;
    imo->camera_yrate = (target - ring->camera.y) / imo->count;
}

void Inv_Ring_MotionCameraPitch(RING_INFO *ring, int16_t target)
{
    IMOTION_INFO *imo = ring->imo;
    imo->camera_pitch_target = target;
    imo->camera_pitch_rate = target / imo->count;
}

void Inv_Ring_MotionItemSelect(RING_INFO *ring, INVENTORY_ITEM *inv_item)
{
    IMOTION_INFO *imo = ring->imo;
    imo->item_ptxrot_target = inv_item->pt_xrot_sel;
    imo->item_ptxrot_rate = inv_item->pt_xrot_sel / imo->count;
    imo->item_xrot_target = inv_item->x_rot_sel;
    imo->item_xrot_rate = inv_item->x_rot_sel / imo->count;
    imo->item_ytrans_target = inv_item->ytrans_sel;
    imo->item_ytrans_rate = inv_item->ytrans_sel / imo->count;
    imo->item_ztrans_target = inv_item->ztrans_sel;
    imo->item_ztrans_rate = inv_item->ztrans_sel / imo->count;
}

void Inv_Ring_MotionItemDeselect(RING_INFO *ring, INVENTORY_ITEM *inv_item)
{
    IMOTION_INFO *imo = ring->imo;
    imo->item_ptxrot_target = 0;
    imo->item_ptxrot_rate = -inv_item->pt_xrot_sel / imo->count;
    imo->item_xrot_target = 0;
    imo->item_xrot_rate = -inv_item->x_rot_sel / imo->count;
    imo->item_ytrans_target = 0;
    imo->item_ytrans_rate = -inv_item->ytrans_sel / imo->count;
    imo->item_ztrans_target = 0;
    imo->item_ztrans_rate = -inv_item->ztrans_sel / imo->count;
}
