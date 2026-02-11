#include <trx/game/inventory_ring/priv.h>

#include <trx/config.h>
#include <trx/game/const.h>
#include <trx/game/game_string.h>
#include <trx/game/input.h>
#include <trx/game/inventory.h>
#include <trx/game/inventory_ring/vars.h>
#include <trx/game/math.h>
#include <trx/game/matrix.h>
#include <trx/game/music.h>
#include <trx/game/objects/names.h>
#include <trx/game/output/state.h>
#include <trx/game/overlay.h>
#include <trx/game/sound.h>
#include <trx/game/ui.h>
#include <trx/strings.h>
#include <trx/version.h>

#include <stdio.h>
#include <string.h>

#define M_CAMERA_Y_OFFSET (-96)
#define M_MANUAL_ROT_RESET_RATE 0.15

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
static bool m_ShowUseItemButton = false;
static char *m_CountText = nullptr;
static size_t m_CountTextCap = 0;
static OBJECT_ID m_RequestedObjectID = NO_OBJECT;

static void M_AdjustRot(int16_t *const rot, const int16_t dest_rot)
{
    const int32_t delta = dest_rot - *rot;
    if (delta != 0) {
        if (delta > 0 && delta < DEG_180) {
            *rot += 1024;
        } else {
            *rot -= 1024;
        }
        *rot &= ~(1024 - 1);
    }
}

static XYZ_32 M_VectorViewFromWorld(const XYZ_32 v_world)
{
    const MATRIX *const m = &g_ViewMatrix;
    return (XYZ_32) {
        .x = (m->_00 * v_world.x + m->_01 * v_world.y + m->_02 * v_world.z)
            >> W2V_SHIFT,
        .y = (m->_10 * v_world.x + m->_11 * v_world.y + m->_12 * v_world.z)
            >> W2V_SHIFT,
        .z = (m->_20 * v_world.x + m->_21 * v_world.y + m->_22 * v_world.z)
            >> W2V_SHIFT,
    };
}

static void M_HandleRequestedObject(INV_RING *const ring)
{
    if (m_RequestedObjectID == NO_OBJECT) {
        return;
    }

    for (int32_t i = 0; i < ring->number_of_objects; i++) {
        const OBJECT_ID object_id = ring->list[i]->object_id;
        if (object_id == m_RequestedObjectID
            && Inv_RequestItem(object_id) > 0) {
            ring->current_object = i;
            break;
        }
    }

    m_RequestedObjectID = NO_OBJECT;
}

void InvRing_AdjustMusicVolume(const INV_RING *const ring)
{
    if (ring->mode == INV_TITLE_MODE) {
        Music_SetVolume(g_Config.audio.music_volume);
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
    ring->rotate_from_object = 0;
    ring->rotate_to_object = 0;
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

    m_ShowExamine = false;
    m_ShowUseItemButton = false;
}

void InvRing_InitInvItem(INVENTORY_ITEM *const inv_item)
{
    inv_item->meshes_drawn = inv_item->meshes_sel;
    inv_item->current_frame = 0;
    inv_item->goal_frame = 0;
    inv_item->manual_rot = g_IDMatrix;
    inv_item->x_rot_pt = 0;
    inv_item->x_rot = inv_item->x_rot_nosel;
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
        -ring->camera.pos.x, M_CAMERA_Y_OFFSET - ring->camera.pos.y,
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

    if (g_TRVersion >= 3) {
        // OG Inv_RingLight() LightCol columns are (sun, spot, dynamic):
        // sun = (3312, 1664, 0);
        // spot = (3312, 3312, 3312);
        // dynamic = (0, 0, 3072) with an ambient of (32, 32, 32).
        const float ambient_u8 = 32.0f / 255.0f;
        const RGB_F ambient = { ambient_u8, ambient_u8, ambient_u8 };
        const RGB_F colors[3] = {
            {
                .r = 3312.0f / 4096.0f,
                .g = 1664.0f / 4096.0f,
                .b = 0.0f,
            },
            {
                .r = 3312.0f / 4096.0f,
                .g = 3312.0f / 4096.0f,
                .b = 3312.0f / 4096.0f,
            },
            {
                .r = 0.0f,
                .g = 0.0f,
                .b = 3072.0f / 4096.0f,
            },
        };
        const XYZ_32 dirs_view[3] = {
            M_VectorViewFromWorld((XYZ_32) {
                .x = 0x4000,
                .y = -0x4000,
                .z = 0x3000,
            }),
            M_VectorViewFromWorld((XYZ_32) {
                .x = -0x4000,
                .y = -0x4000,
                .z = 0x3000,
            }),
            M_VectorViewFromWorld(
                (XYZ_32) { .x = 0, .y = 0x2000, .z = 0x3000 }),
        };
        Output_SetTR3Light(ambient, colors, dirs_view);
    }
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
    ring->rotate_from_object = ring->current_object;
    if (ring->current_object <= 0) {
        ring->target_object = ring->number_of_objects - 1;
    } else {
        ring->target_object = ring->current_object - 1;
    }
    ring->rotate_to_object = ring->target_object;
    ring->rot_count = INV_RING_ROTATE_DURATION;
    ring->rot_adder = ring->rot_adder_l;
}

void InvRing_RotateRight(INV_RING *const ring)
{
    ring->rotating = true;
    ring->rotate_from_object = ring->current_object;
    if (ring->current_object + 1 >= ring->number_of_objects) {
        ring->target_object = 0;
    } else {
        ring->target_object = ring->current_object + 1;
    }
    ring->rotate_to_object = ring->target_object;
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
    case O_STOPWATCH_OPTION:
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
    if (inv_item->object_id == O_PASSPORT_OPTION
        || inv_item->object_id == O_GLOBE_SELECT_OPTION) {
        return;
    }
    Overlay_SetBottomText((OVERLAY_TEXT) {
        .kind = UI_OVERLAY_TEXT_OBJECT_NAME,
        .object_id = inv_item->object_id,
        .fmt_gs_key = GS_ID(INVENTORY_RING_OBJECT_NAME_FMT),
    });
}

void InvRing_ShowItemQuantity(const char *const fmt, const int32_t qty)
{
    const char *const full_fmt =
        String_FormatStatic(GS(INVENTORY_RING_ITEM_COUNT_FMT), fmt);
    String_FormatInto(&m_CountText, &m_CountTextCap, full_fmt, qty);
}

void InvRing_DrawUI(INV_RING *const ring)
{
    if (m_ShowExamine) {
        UI_BeginModal(0.5f, 0.85f);
        UI_BeginStack(UI_STACK_HORIZONTAL);
        UI_ButtonLabel(INPUT_ROLE_LOOK, GS(ACTION_EXAMINE_ITEM));
        if (m_ShowUseItemButton) {
            UI_Spacer(60.0f, 0.0f);
            UI_ButtonLabel(INPUT_ROLE_ACTION, GS(ACTION_USE_ITEM));
        }
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
    Overlay_SetBottomText((OVERLAY_TEXT) { 0 });
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
        Overlay_SetTopText((OVERLAY_TEXT) {
            .kind = UI_OVERLAY_TEXT_GS_KEY,
            .gs_key = GS_ID(INVENTORY_RING_HEADING_INVENTORY),
            .fmt_gs_key = GS_ID(INVENTORY_RING_HEADING_FMT),
        });
        break;
    case RT_OPTION:
        if (ring->mode == INV_DEATH_MODE) {
            Overlay_SetTopText((OVERLAY_TEXT) {
                .kind = UI_OVERLAY_TEXT_GS_KEY,
                .gs_key = GS_ID(INVENTORY_RING_HEADING_GAME_OVER),
                .fmt_gs_key = GS_ID(INVENTORY_RING_HEADING_FMT),
            });
        } else {
            Overlay_SetTopText((OVERLAY_TEXT) {
                .kind = UI_OVERLAY_TEXT_GS_KEY,
                .gs_key = GS_ID(INVENTORY_RING_HEADING_OPTION),
                .fmt_gs_key = GS_ID(INVENTORY_RING_HEADING_FMT),
            });
        }
        break;
    case RT_KEYS:
        Overlay_SetTopText((OVERLAY_TEXT) {
            .kind = UI_OVERLAY_TEXT_GS_KEY,
            .gs_key = GS_ID(INVENTORY_RING_HEADING_ITEMS),
            .fmt_gs_key = GS_ID(INVENTORY_RING_HEADING_FMT),
        });
        break;
    case RT_GLOBE_SELECT:
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
    Overlay_SetTopText((OVERLAY_TEXT) { 0 });
    Overlay_ShowArrow(UI_OVERLAY_ARROW_TL, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_TR, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BL, false);
    Overlay_ShowArrow(UI_OVERLAY_ARROW_BR, false);
}

void InvRing_ShowExamine(const bool show, const OBJECT_ID object_id)
{
    m_ShowExamine = show;
    m_ShowUseItemButton = show;
    if (show) {
        switch (object_id) {
        case O_QUEST_ITEM_1:
        case O_QUEST_ITEM_2:
        case O_QUEST_ITEM_3:
        case O_QUEST_ITEM_4:
        case O_QUEST_OPTION_1:
        case O_QUEST_OPTION_2:
        case O_QUEST_OPTION_3:
        case O_QUEST_OPTION_4:
        case O_PICKUP_OPTION_1:
        case O_PICKUP_OPTION_2:
            m_ShowUseItemButton = false;
            break;
        default:
            break;
        }
    }
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

        if (inv_item->has_manual_rot) {
            return;
        }

        for (int32_t i = 0; i < num_frames; i++) {
            M_AdjustRot(&inv_item->y_rot, inv_item->y_rot_sel);
            Matrix_Slerp3x3_M(
                &inv_item->manual_rot, &g_IDMatrix, M_MANUAL_ROT_RESET_RATE);
        }
    } else if (
        ring->number_of_objects == 1
        || (!g_Input.menu_right && !g_Input.menu_left)) {
        for (int32_t i = 0; i < num_frames; i++) {
            inv_item->y_rot += 256;
        }
    }
}
