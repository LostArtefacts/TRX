#include "game/inventory_ring/priv.h"

#include "config.h"
#include "game/const.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory_ring/vars.h"
#include "game/math.h"
#include "game/music.h"
#include "game/objects/names.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/sound.h"
#include "game/ui.h"
#include "strings.h"

#include <stdio.h>
#include <string.h>

#define RING_CAMERA_Y_OFFSET (-96)

typedef enum {
    // clang-format off
    PASS_MESH_SPINE    = 1 << 0,
    PASS_MESH_FRONT    = 1 << 1,
    PASS_MESH_IN_FRONT = 1 << 2,
    PASS_MESH_PAGE_2   = 1 << 3,
    PASS_MESH_BACK     = 1 << 4,
    PASS_MESH_IN_BACK  = 1 << 5,
    PASS_MESH_PAGE_1   = 1 << 6,
    PASS_MESH_COMMON   = PASS_MESH_SPINE | PASS_MESH_BACK | PASS_MESH_IN_BACK | PASS_MESH_FRONT,
    // clang-format on
} PASS_MESH;

static bool m_ShowExamine = false;
static char *m_CountText = nullptr;
static size_t m_CountTextCap = 0;
static OBJECT_ID m_RequestedObjectID = NO_OBJECT;

static void M_HandleRequestedObject(INV_RING *const ring)
{
    if (m_RequestedObjectID == NO_OBJECT) {
        return;
    }

    for (int32_t i = 0; i < ring->number_of_objects; i++) {
        const OBJECT_ID item_id = ring->list[i]->object_id;
        if (item_id == m_RequestedObjectID && Inv_RequestItem(item_id) > 0) {
            ring->current_object = i;
            break;
        }
    }

    m_RequestedObjectID = NO_OBJECT;
}

void InvRing_AdjustMusicVolume(const INV_RING *const ring)
{
    if (ring->mode == INV_TITLE_MODE) {
        return;
    }
    const bool is_ambient =
        Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();
    const double base_volume = is_ambient ? g_Config.audio.ambient_volume
                                          : g_Config.audio.music_volume;
    const double multiplier = is_ambient
        ? g_Config.audio.inventory_ambient_volume
        : g_Config.audio.inventory_music_volume;
    Music_SetVolume(base_volume * multiplier);

    Sound_ResetAmbient();
    Sound_UpdateEffects();
}

void InvRing_SetRequestedObjectID(const OBJECT_ID obj_id)
{
    m_RequestedObjectID = obj_id;
}

void InvRing_InitRing(
    INV_RING *const ring, const RING_TYPE type, INVENTORY_ITEM **const list,
    const int16_t qty, const int16_t current)
{
    ring->type = type;
    ring->list = list;
    ring->radius = 0;
    ring->number_of_objects = qty;
    ring->current_object = current;
    ring->angle_adder = DEG_360 / qty;

    ring->is_pass_open = false;
    ring->is_demo_needed = false;
    ring->has_spun_out = false;

    M_HandleRequestedObject(ring);

    if (ring->mode == INV_TITLE_MODE) {
        ring->camera_pitch = 1024;
    } else {
        ring->camera_pitch = 0;
    }

    ring->rotating = false;
    ring->rot_count = 0;
    ring->target_object = 0;
    ring->rot_adder = 0;
    ring->rot_adder_l = 0;
    ring->rot_adder_r = 0;

    ring->camera.pos.x = 0;
    ring->camera.pos.y = INV_RING_CAMERA_START_HEIGHT;
    ring->camera.pos.z = 896;
    ring->camera.rot.x = 0;
    ring->camera.rot.y = 0;
    ring->camera.rot.z = 0;

    InvRing_MotionInit(ring, RNG_OPENING, RNG_OPEN, INV_RING_OPEN_FRAMES);
    InvRing_MotionRadius(ring, INV_RING_RADIUS);
    InvRing_MotionCameraPos(ring, INV_RING_CAMERA_HEIGHT);
    InvRing_MotionRotation(
        ring, INV_RING_OPEN_ROTATION,
        -DEG_90 - ring->current_object * ring->angle_adder);

    ring->ring_pos.pos.x = 0;
    ring->ring_pos.pos.y = 0;
    ring->ring_pos.pos.z = 0;
    ring->ring_pos.rot.x = 0;
    ring->ring_pos.rot.y = ring->motion.rotate_target - INV_RING_OPEN_ROTATION;
    ring->ring_pos.rot.z = 0;

    ring->light.x = -1536;
    ring->light.y = 256;
    ring->light.z = 1024;

    ring->motion_timer.type = CLOCK_TIMER_SIM;
    ClockTimer_Sync(&ring->motion_timer);
}

void InvRing_InitInvItem(INVENTORY_ITEM *const inv_item)
{
    inv_item->meshes_drawn = inv_item->meshes_sel;
    inv_item->current_frame = 0;
    inv_item->goal_frame = 0;
    inv_item->x_rot_pt = 0;
    inv_item->x_rot = 0;
    inv_item->y_rot = 0;
    inv_item->y_trans = 0;
    inv_item->z_trans = 0;
    inv_item->action = ACTION_USE;
    if (inv_item->object_id == O_PASSPORT_OPTION) {
        inv_item->object_id = O_PASSPORT_CLOSED;
    }
}

void InvRing_GetView(
    const INV_RING *const ring, XYZ_32 *const out_pos, XYZ_16 *const out_rot)
{
    int16_t angles[2];
    Math_GetVectorAngles(
        -ring->camera.pos.x, RING_CAMERA_Y_OFFSET - ring->camera.pos.y,
        ring->radius - ring->camera.pos.z, angles);
    out_pos->x = ring->camera.pos.x;
    out_pos->y = ring->camera.pos.y;
    out_pos->z = ring->camera.pos.z;
    out_rot->x = angles[1] + ring->camera_pitch;
    out_rot->y = angles[0];
    out_rot->z = 0;
}

void InvRing_Light(const INV_RING *const ring)
{
    int16_t angles[2];
    Math_GetVectorAngles(ring->light.x, ring->light.y, ring->light.z, angles);
    Output_SetLightDivider(0x6000);
    Output_RotateLight(angles[1], angles[0]);
}

void InvRing_CalcAdders(INV_RING *const ring, const int16_t rotation_duration)
{
    ring->angle_adder = DEG_360 / ring->number_of_objects;
    ring->rot_adder_l = ring->angle_adder / rotation_duration;
    ring->rot_adder_r = -ring->angle_adder / rotation_duration;
}

void InvRing_DoMotions(INV_RING *const ring)
{
    INV_RING_MOTION *const motion = &ring->motion;

    if (motion->count != 0) {
        ring->radius += motion->radius_rate;
        ring->camera.pos.y += motion->camera_y_rate;
        ring->ring_pos.rot.y += motion->rotate_rate;
        ring->camera_pitch += motion->camera_pitch_rate;

        INVENTORY_ITEM *const inv_item = ring->list[ring->current_object];
        inv_item->x_rot_pt += motion->item_pt_x_rot_rate;
        inv_item->x_rot += motion->item_x_rot_rate;
        inv_item->y_trans += motion->item_y_trans_rate;
        inv_item->z_trans += motion->item_z_trans_rate;

        motion->count--;
        if (motion->count == 0) {
            motion->status = motion->status_target;

            if (motion->radius_rate != 0) {
                motion->radius_rate = 0;
                ring->radius = motion->radius_target;
            }
            if (motion->camera_y_rate != 0) {
                motion->camera_y_rate = 0;
                ring->camera.pos.y = motion->camera_y_target;
            }
            if (motion->rotate_rate != 0) {
                motion->rotate_rate = 0;
                ring->ring_pos.rot.y = motion->rotate_target;
            }
            if (motion->item_pt_x_rot_rate != 0) {
                motion->item_pt_x_rot_rate = 0;
                inv_item->x_rot_pt = motion->item_pt_x_rot_target;
            }
            if (motion->item_x_rot_rate != 0) {
                motion->item_x_rot_rate = 0;
                inv_item->x_rot = motion->item_x_rot_target;
            }
            if (motion->item_y_trans_rate != 0) {
                motion->item_y_trans_rate = 0;
                inv_item->y_trans = motion->item_y_trans_target;
            }
            if (motion->item_z_trans_rate != 0) {
                motion->item_z_trans_rate = 0;
                inv_item->z_trans = motion->item_z_trans_target;
            }
            if (motion->camera_pitch_rate != 0) {
                motion->camera_pitch_rate = 0;
                ring->camera_pitch = motion->camera_pitch_target;
            }
        }
    }

    if (ring->rotating) {
        ring->ring_pos.rot.y += ring->rot_adder;
        ring->rot_count--;

        if (ring->rot_count == 0) {
            ring->current_object = ring->target_object;
            ring->ring_pos.rot.y =
                -DEG_90 - ring->target_object * ring->angle_adder;
            ring->rotating = false;
        }
    }
}

void InvRing_RotateLeft(INV_RING *const ring)
{
    ring->rotating = true;
    if (ring->current_object <= 0) {
        ring->target_object = ring->number_of_objects - 1;
    } else {
        ring->target_object = ring->current_object - 1;
    }
    ring->rot_count = INV_RING_ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_l;
}

void InvRing_RotateRight(INV_RING *const ring)
{
    ring->rotating = true;
    if (ring->current_object + 1 >= ring->number_of_objects) {
        ring->target_object = 0;
    } else {
        ring->target_object = ring->current_object + 1;
    }
    ring->rot_count = INV_RING_ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_r;
}

void InvRing_MotionInit(
    INV_RING *const ring, const RING_STATUS status,
    const RING_STATUS status_target, const int16_t frames)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->count = frames;
    motion->status = status;
    motion->status_target = status_target;

    motion->radius_target = 0;
    motion->radius_rate = 0;
    motion->camera_y_target = 0;
    motion->camera_y_rate = 0;
    motion->camera_pitch_target = 0;
    motion->camera_pitch_rate = 0;
    motion->rotate_target = 0;
    motion->rotate_rate = 0;
    motion->item_pt_x_rot_target = 0;
    motion->item_pt_x_rot_rate = 0;
    motion->item_x_rot_target = 0;
    motion->item_x_rot_rate = 0;
    motion->item_y_trans_target = 0;
    motion->item_y_trans_rate = 0;
    motion->item_z_trans_target = 0;
    motion->item_z_trans_rate = 0;

    motion->misc = 0;
}

void InvRing_MotionSetup(
    INV_RING *const ring, const RING_STATUS status,
    const RING_STATUS status_target, const int16_t frames)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->count = frames;
    motion->status = status;
    motion->status_target = status_target;
    motion->radius_rate = 0;
    motion->camera_y_rate = 0;
}

void InvRing_MotionRadius(INV_RING *const ring, const int16_t target)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->radius_target = target;
    motion->radius_rate = (target - ring->radius) / motion->count;
}

void InvRing_MotionRotation(
    INV_RING *const ring, const int16_t rotation, const int16_t target)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->rotate_target = target;
    motion->rotate_rate = rotation / motion->count;
}

void InvRing_MotionCameraPos(INV_RING *const ring, const int16_t target)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->camera_y_target = target;
    motion->camera_y_rate = (target - ring->camera.pos.y) / motion->count;
}

void InvRing_MotionCameraPitch(INV_RING *const ring, const int16_t target)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->camera_pitch_target = target;
    motion->camera_pitch_rate = target / motion->count;
}

void InvRing_MotionItemSelect(
    INV_RING *const ring, const INVENTORY_ITEM *const inv_item)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->item_pt_x_rot_target = inv_item->x_rot_pt_sel;
    motion->item_pt_x_rot_rate = inv_item->x_rot_pt_sel / motion->count;
    motion->item_x_rot_target = inv_item->x_rot_sel;
    motion->item_x_rot_rate =
        (inv_item->x_rot_sel - inv_item->x_rot_nosel) / motion->count;
    motion->item_y_trans_target = inv_item->y_trans_sel;
    motion->item_y_trans_rate = inv_item->y_trans_sel / motion->count;
    motion->item_z_trans_target = inv_item->z_trans_sel;
    motion->item_z_trans_rate = inv_item->z_trans_sel / motion->count;
}

void InvRing_MotionItemDeselect(
    INV_RING *const ring, const INVENTORY_ITEM *const inv_item)
{
    INV_RING_MOTION *const motion = &ring->motion;
    motion->item_pt_x_rot_target = 0;
    motion->item_pt_x_rot_rate = -(inv_item->x_rot_pt_sel / motion->count);
    motion->item_x_rot_target = inv_item->x_rot_nosel;
    motion->item_x_rot_rate =
        (inv_item->x_rot_nosel - inv_item->x_rot_sel) / motion->count;
    motion->item_y_trans_target = 0;
    motion->item_y_trans_rate = -(inv_item->y_trans_sel / motion->count);
    motion->item_z_trans_target = 0;
    motion->item_z_trans_rate = -(inv_item->z_trans_sel / motion->count);
}

void InvRing_SelectMeshes(INVENTORY_ITEM *const inv_item)
{
    switch (inv_item->object_id) {
    case O_PASSPORT_OPTION: {
        struct {
            int32_t frame;
            uint32_t meshes;
        } frame_map[] = {
            { 14, PASS_MESH_IN_FRONT | PASS_MESH_PAGE_1 },
            { 18, PASS_MESH_IN_FRONT | PASS_MESH_PAGE_1 | PASS_MESH_PAGE_2 },
            { 19, PASS_MESH_PAGE_1 | PASS_MESH_PAGE_2 },
            { 23, PASS_MESH_PAGE_1 | PASS_MESH_PAGE_2 | PASS_MESH_IN_BACK },
            { 28, PASS_MESH_PAGE_2 | PASS_MESH_IN_BACK },
            { 29, 0 },
            { -1, -1 }, // sentinel
        };

        for (int32_t i = 0; frame_map[i].frame != -1; i++) {
            if (inv_item->current_frame <= frame_map[i].frame) {
                inv_item->meshes_drawn = PASS_MESH_COMMON | frame_map[i].meshes;
                break;
            }
        }
        break;
    }

    case O_COMPASS_OPTION:
        if (inv_item->current_frame == 0 || inv_item->current_frame >= 18) {
            inv_item->meshes_drawn = inv_item->meshes_sel;
        } else {
            inv_item->meshes_drawn = -1;
        }
        break;

    default:
        inv_item->meshes_drawn = -1;
        break;
    }
}

void InvRing_ShowItemName(const INVENTORY_ITEM *const inv_item)
{
    if (inv_item->object_id == O_PASSPORT_OPTION) {
        return;
    }
    Overlay_SetBottomTextPtr(Object_GetNamePtr(inv_item->object_id), false);
}

void InvRing_ShowItemQuantity(const char *const fmt, const int32_t qty)
{
    const char *const tmp = String_FormatStatic(fmt, qty);
    String_StylizeSmallDigitsInto(&m_CountText, &m_CountTextCap, tmp);
}

void InvRing_DrawUI(INV_RING *const ring)
{
    if (m_ShowExamine) {
        UI_BeginModal(0.5f, 0.8f);
        UI_BeginStack(UI_STACK_HORIZONTAL);
        UI_ButtonLabel(INPUT_ROLE_LOOK, GS(ACTION_EXAMINE_ITEM));
        UI_Spacer(60.0f, 0.0f);
        UI_ButtonLabel(INPUT_ROLE_ACTION, GS(ACTION_USE_ITEM));
        UI_EndStack();
        UI_EndModal();
    }

    if (m_CountText != nullptr && m_CountText[0] != '\0') {
        UI_BeginModal(0.5f, 1.0f);
        UI_BeginOffset(64.0f, -56.0f);
        UI_Label(m_CountText);
        UI_EndOffset();
        UI_EndModal();
    }
}

void InvRing_RemoveItemTexts(void)
{
    Overlay_SetBottomText(nullptr, false);
    if (m_CountText != nullptr) {
        strcpy(m_CountText, "");
    }
}

void InvRing_ShowHeader(INV_RING *const ring)
{
    if (ring->mode == INV_TITLE_MODE) {
        return;
    }

    switch (ring->type) {
    case RT_MAIN:
        Overlay_SetTopTextPtr(GS_PTR(HEADING_INVENTORY), false);
        break;
    case RT_OPTION:
        if (ring->mode == INV_DEATH_MODE) {
            Overlay_SetTopTextPtr(GS_PTR(HEADING_GAME_OVER), false);
        } else {
            Overlay_SetTopTextPtr(GS_PTR(HEADING_OPTION), false);
        }
        break;
    case RT_KEYS:
        Overlay_SetTopTextPtr(GS_PTR(HEADING_ITEMS), false);
        break;
    case RT_NUMBER_OF:
        break;
    }

    if (ring->mode != INV_GAME_MODE) {
        return;
    }

    const bool show_up_arrow = ring->type == RT_OPTION
        || (ring->type == RT_MAIN && g_InvRing_Source[RT_KEYS].count > 0);
    const bool show_bottom_arrow = ring->type == RT_KEYS
        || (ring->type == RT_MAIN && !InvRing_IsOptionLockedOut());

    Overlay_ShowArrow(UI_OVERLAY_ARROW_TL, show_up_arrow);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_TR, show_up_arrow);

    Overlay_ShowArrow(UI_OVERLAY_ARROW_BL, show_bottom_arrow);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BR, show_bottom_arrow);
}

void InvRing_RemoveHeader(void)
{
    Overlay_SetTopText(nullptr, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_TL, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_TR, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BL, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BR, false);
}

void InvRing_ShowExamine(const bool show)
{
    m_ShowExamine = show;
}

bool InvRing_CanExamine(void)
{
    return g_Config.gameplay.enable_item_examining && m_ShowExamine;
}

void InvRing_ShowVersionText(void)
{
    Overlay_ShowVersion(true);
}

void InvRing_RemoveVersionText(void)
{
    Overlay_ShowVersion(false);
}

void InvRing_UpdateInventoryItem(
    const INV_RING *const ring, INVENTORY_ITEM *const inv_item,
    const int32_t num_frames)
{
    if (inv_item != ring->list[ring->current_object]) {
        for (int32_t i = 0; i < num_frames; i++) {
            if (inv_item->y_rot < 0) {
                inv_item->y_rot += 256;
            } else if (inv_item->y_rot > 0) {
                inv_item->y_rot -= 256;
            }
        }
    } else if (ring->rotating) {
        for (int32_t i = 0; i < num_frames; i++) {
            if (inv_item->y_rot > 0) {
                inv_item->y_rot -= 512;
            } else if (inv_item->y_rot < 0) {
                inv_item->y_rot += 512;
            }
        }
    } else if (
        ring->motion.status == RNG_SELECTED
        || ring->motion.status == RNG_DESELECTING
        || ring->motion.status == RNG_SELECTING
        || ring->motion.status == RNG_DESELECT
        || ring->motion.status == RNG_CLOSING_ITEM) {
        for (int32_t i = 0; i < num_frames; i++) {
            const int32_t delta = inv_item->y_rot_sel - inv_item->y_rot;
            if (delta != 0) {
                if (delta > 0 && delta < DEG_180) {
                    inv_item->y_rot += 1024;
                } else {
                    inv_item->y_rot -= 1024;
                }
                inv_item->y_rot &= ~(1024 - 1);
            }
        }
    } else if (
        ring->number_of_objects == 1
        || (!g_Input.menu_right && !g_Input.menu_left)) {
        for (int32_t i = 0; i < num_frames; i++) {
            inv_item->y_rot += 256;
        }
    }
}
