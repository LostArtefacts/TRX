#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/game/anims.h>
#include <trx/game/camera.h>
#include <trx/game/clock.h>
#include <trx/game/const.h>
#include <trx/game/creature.h>
#include <trx/game/fader.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/lara/common.h>
#include <trx/game/lara/skin/common.h>
#include <trx/game/music.h>
#include <trx/game/objects/draw.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/rooms/floor_data.h>
#include <trx/game/savegame.h>
#include <trx/game/spawn.h>
#include <trx/game/stats.h>
#include <trx/game/viewport.h>
#include <trx/game/waypoint.h>
#include <trx/version.h>

// clang-format off
#define M_HIT_POINTS          15
#define M_RADIUS              (STEP_L / 2)
#define M_MAX_WAYPOINTS       64
#define M_CUT_INDEX_NONE      255
#define M_FLYBY_TALK_TRACK    80
#define M_HEAD_MESH           21
#define M_WAYPOINT_UNSET      (-1)
#define M_STEP_AHEAD          808

// How a scene of his frames the camera. The flag word says whether the
// position and the target are places of their own or offsets from where the
// camera already was, and its low half carries the field of view.
#define M_CUT_ABSOLUTE_POS    0x40000
#define M_CUT_ABSOLUTE_TARGET 0x80000
#define M_CUT_TARGET_SELF     0x20000
#define M_CUT_FOV_MASK        0xFFFF

#define M_SWAP_MESHES         0x40080
#define M_SWAP_BOOK           0x8000

#define M_RANGE_MARKER        SQUARE(WALL_L / 8)
#define M_RANGE_WALK          SQUARE(WALL_L * 5 / 8)
#define M_RANGE_ATTACK        SQUARE(WALL_L)
#define M_RANGE_TARGET        (2 * SQUARE(WALL_L))
#define M_RANGE_RUN           SQUARE(WALL_L * 3)
#define M_RANGE_LOST          SQUARE(WALL_L * 5)
// clang-format on

typedef enum {
    // clang-format off
    M_ANIM_STAND                = 4,
    M_ANIM_GET_KNIFE            = 11,
    M_ANIM_JUMP_FORWARD_START   = 22,
    M_ANIM_JUMP_FORWARD_1_BLOCK = 23,
    M_ANIM_JUMP_FORWARD_2_BLOCK = 25,
    M_ANIM_VAULT_4_CLICKS       = 27,
    M_ANIM_VAULT_3_CLICKS       = 28,
    M_ANIM_VAULT_2_CLICKS       = 29,
    M_ANIM_DROP_4_CLICKS        = 35,
    M_ANIM_DROP_8_CLICKS        = 36,
    M_ANIM_JUMP_TO_HANG         = 37,
    M_ANIM_DROP_3_CLICKS        = 41,
    M_ANIM_DROP_2_CLICKS        = 42,
    M_ANIM_HANGING              = 43,
    M_ANIM_RAISE_ARM            = 47,
    M_ANIM_GRAB_CLIMB_ON        = 52,
    // clang-format on
} M_ANIM;

typedef enum {
    M_STATE_NULL,
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_RUN,
    M_STATE_MONKEY_IDLE,
    M_STATE_MONKEY_FORWARD,
    M_STATE_GET_KNIFE,
    M_STATE_CHECK_GROUND,
    M_STATE_TALK_1,
    M_STATE_TALK_2,
    M_STATE_TALK_3,
    M_STATE_READ_BOOK,
    M_STATE_STOP_LARA,
    M_STATE_USHER_FAR,
    M_STATE_USHER_NEAR,
    M_STATE_JUMP_FORWARD_1_BLOCK,
    M_STATE_JUMP_FORWARD_2_BLOCK,
    M_STATE_VAULT_4_CLICKS,
    M_STATE_VAULT_3_CLICKS,
    M_STATE_VAULT_2_CLICKS,
    M_STATE_USE_SWITCH,
    M_STATE_ATTACK_HIGH,
    M_STATE_TURN_LEFT,
    M_STATE_DROP_2_CLICKS,
    M_STATE_DROP_3_CLICKS,
    M_STATE_DROP_4_CLICKS,
    M_STATE_DROP_8_CLICKS,
    M_STATE_HANGING,
    M_STATE_SHIMMY_RIGHT,
    M_STATE_JUMP_TO_HANG,
    M_STATE_CLIMB_ON,
    M_STATE_ATTACK_LOW,
    M_STATE_RAISED_ARM,
    M_STATE_GRAB_CLIMB_ON,
    M_STATE_HOP_BACK,
    M_STATE_TURN_RIGHT,
    M_STATE_CORRECT_POSITION_FRONT,
    M_STATE_CORRECT_POSITION_BACK,
} M_STATE;

typedef struct {
    // Which of his meshes come from the swap object, which the original keeps
    // in a field of its own. It is not ITEM.mesh_bits: that says which meshes
    // are drawn, and writing this into it leaves almost none of him.
    int32_t swap_bits;
    // The waypoint he is working towards, which is what he compares Lara's
    // own progress against and what names the line he says. It starts at the
    // number carried by the marker he is placed on.
    int16_t waypoint;
    // What he is doing while stopped. The original gives its three values no
    // names: 0 while he is going about his business, 2 once a scene of his has
    // just finished, and 6 while he stands at a marker facing the way it
    // faces.
    int16_t hold;
    // Set once Lara has finished climbing at the waypoint that waits for it,
    // so that it is only waited for the once.
    bool climb_done;
    // The phase of a scene, which the original keeps in the item's
    // trigger_flags and seeds from the number the level gave it.
    int16_t cut_phase;
    // Whether the scene's track has been heard playing. Playing a track does
    // not make it current in the same frame, and without this the scene reads
    // the silence before it starts as the silence after it ends.
    bool cut_heard;
    // How long the line he says lasts and how far into it the scene is, both
    // in frames. A length of zero is one the audio file could not give.
    int32_t cut_length;
    int32_t cut_timer;
    // Which of his lectures the player has heard, one bit per waypoint.
    uint64_t cut_played;
    // Whether he waits for Lara and lectures her at his markers rather than
    // running them on his own.
    bool guides_lara;
} M_PRIV;

typedef struct {
    XYZ_32 camera_pos;
    XYZ_32 camera_target;
    int32_t flags;
} M_CUT_DATA;

static FADER m_Fader = {};

static BITE m_Bite = { { 0, 35, 130 }, 18 };

// Which of the scenes below a waypoint frames, indexed by Waypoint_GetPad().
static uint8_t m_CutIndices[68] = {
    1,   2,   255, 0,   3,   255, 0,  4,  0,   0,   0,   0,   5,   6,
    0,   0,   0,   255, 0,   0,   7,  0,  255, 255, 0,   8,   0,   255,
    255, 255, 255, 255, 255, 255, 9,  0,  10,  255, 255, 255, 255, 255,
    255, 0,   255, 255, 0,   0,   11, 12, 255, 255, 255, 0,   255, 255,
    0,   0,   13,  14,  255, 0,   0,  0,  0,   0,   0,   0
};

static M_CUT_DATA m_CutData[15] = {
    { { 256, -386, 256 }, { 0, 0, 0 }, 0x20050 },
    { { 8845, 453, 83931 }, { 0, 0, 0 }, 0x40000 },
    { { 0, -1024, 0 }, { 0, 0, 0 }, 0x20000 },
    { { 17435, 2500, 61472 }, { 0, 0, 0 }, 0x40000 },
    { { 30199, 1029, 51933 }, { 0, 0, 0 }, 0x60000 },
    { { 38047, 468, 52008 }, { 27190, 1280, 60752 }, 0xC0000 },
    { { 37130, 314, 61563 }, { 41883, -1291, 59413 }, 0xC0000 },
    { { 55203, -3083, 53155 }, { 0, 0, 0 }, 0x40000 },
    { { 60944, 601, 50535 }, { 62163, -432, 47405 }, 0xC0000 },
    { { 94354, 366, 65718 }, { 92461, -432, 60717 }, 0xC0000 },
    { { 94567, -2081, 63235 }, { 0, 0, 0 }, 0x60000 },
    { { 79376, 219, 30345 }, { 0, 0, 0 }, 0x40000 },
    { { 78067, -3375, 36470 }, { 0, 0, 0 }, 0x40000 },
    { { 72648, 195, 41947 }, { 0, 0, 0 }, 0x40000 },
    { { 66935, -3372, 40726 }, { 0, 0, 0 }, 0x40000 }
};

static int16_t m_CutTracks[M_MAX_WAYPOINTS] = {
    31, 62, -1, 30, 24, -1, 17, 44, 1,  46, 3,  11, 10, 45, 13, 4,
    39, -1, 67, 34, 61, -1, -1, -1, 70, 28, -1, -1, -1, -1, -1, -1,
    -1, -1, 68, 26, 43, -1, -1, -1, -1, -1, -1, 37, -1, -1, 36, 21,
    25, 23, -1, -1, -1, 38, -1, -1, 36, 21, 25, 23, -1, -1, -1, -1
};

// Where the camera was before a scene took it, and what it was doing.
static CAMERA_INFO m_OldCamera = {};

// The speech head he wears, and how long the flyby's track has been playing,
// which together decide whose lips move during it.
static OBJECT_ID m_SwappedHead = NO_OBJECT;
static int32_t m_TalkTimer = 0;

// What he knows about his marker and about Lara. The original keeps both
// across frames, and the guide reads them again after a scene of his.
static AI_INFO m_AI = {};
static AI_INFO m_LaraAI = {};

static bool M_IsFadedOut(void)
{
    return !Fader_IsActive(&m_Fader) && Fader_GetCurrentValue(&m_Fader) >= 1.0f;
}

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static M_PRIV *M_GetPriv(const ITEM *const item)
{
    return item->priv;
}

static bool M_IsWaypoint(const int32_t num)
{
    return num >= 0 && num < M_MAX_WAYPOINTS;
}

static int16_t M_GetTrack(const int32_t waypoint)
{
    return M_IsWaypoint(waypoint) ? m_CutTracks[waypoint] : -1;
}

static bool M_IsCutPlayed(const M_PRIV *const p, const int32_t waypoint)
{
    return M_IsWaypoint(waypoint) ? (p->cut_played & (1ULL << waypoint)) != 0
                                  : true;
}

static void M_SetCutPlayed(M_PRIV *const p, const int32_t waypoint)
{
    if (M_IsWaypoint(waypoint)) {
        p->cut_played |= 1ULL << waypoint;
    }
}

static bool M_IsValidPos(const XYZ_32 pos, int16_t room_num)
{
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t height = Room_GetHeight(sector, pos);
    const int32_t ceiling = Room_GetCeiling(sector, pos);
    return height != NO_HEIGHT && pos.y <= height && pos.y >= ceiling;
}

static void M_SetCutCamera(const ITEM *const item)
{
    m_OldCamera = g_Camera;
    g_Camera.type = CAM_CHASE;
    g_Camera.speed = 1;
    Camera_Invalidate();
    Camera_Update();
    m_OldCamera.target = g_Camera.target;
    m_OldCamera.pos = g_Camera.pos;
    g_Camera.underwater = false;
    const int32_t pad = Waypoint_GetPad();
    if (pad < 0 || pad >= (int32_t)sizeof(m_CutIndices)) {
        return;
    }
    const uint8_t cut_idx = m_CutIndices[pad];
    if (cut_idx >= (uint8_t)(sizeof(m_CutData) / sizeof(m_CutData[0]))) {
        return;
    }
    g_Camera.type = CAM_CINEMATIC;
    g_Camera.flags = CF_BLOCK_UPDATE;
    const M_CUT_DATA *const cut = &m_CutData[cut_idx];
    const int32_t flags = cut->flags;

    if ((flags & M_CUT_FOV_MASK) != 0) {
        Viewport_AlterFOV(
            (int16_t)(DEG_1 * (flags & M_CUT_FOV_MASK)), FOV_MODE_CUTSCENE);
    }

    if ((flags & M_CUT_ABSOLUTE_POS) != 0) {
        g_Camera.pos.x = cut->camera_pos.x;
        g_Camera.pos.y = cut->camera_pos.y;
        g_Camera.pos.z = cut->camera_pos.z;
    } else {
        g_Camera.pos.x += cut->camera_pos.x;
        g_Camera.pos.y += cut->camera_pos.y;
        g_Camera.pos.z += cut->camera_pos.z;
    }

    if ((flags & M_CUT_ABSOLUTE_TARGET) != 0) {
        g_Camera.target.x = cut->camera_target.x;
        g_Camera.target.y = cut->camera_target.y;
        g_Camera.target.z = cut->camera_target.z;
    } else {
        g_Camera.target.x += cut->camera_target.x;
        g_Camera.target.y += cut->camera_target.y;
        g_Camera.target.z += cut->camera_target.z;
    }

    if ((flags & M_CUT_TARGET_SELF) != 0) {
        g_Camera.target.x = item->pos.x;
        g_Camera.target.y = item->pos.y - 256;
        g_Camera.target.z = item->pos.z;
    }

    const int16_t cam_room = Room_GetIndexFromPos(g_Camera.pos.pos);
    if (cam_room == NO_ROOM || !M_IsValidPos(g_Camera.pos.pos, cam_room)) {
        g_Camera.pos = m_OldCamera.pos;
    } else {
        g_Camera.pos.room_num = cam_room;
    }
}

static void M_ClearCutCamera(void)
{
    g_Camera.pos = m_OldCamera.pos;
    g_Camera.target = m_OldCamera.target;
    g_Camera.speed = 1;
    g_Camera.type = m_OldCamera.type;
    g_Camera.flags = m_OldCamera.flags;
    Viewport_AlterFOV(-1, FOV_MODE_GAME);
}

// The AI object carrying this OCB is where he walks to and stands. TR4's
// markers are items like TR1-3's, so the enemy points at one rather than at a
// copy of it inside the creature.
static void M_GetAIEnemy(CREATURE *info, int32_t ocb)
{
    ITEM *const target = Creature_FindAIObjectByOCB(ocb);
    if (target != nullptr) {
        info->enemy = target;
    }
}

// The lecture lasts as long as the line he says. Its length comes from the
// audio file rather than from the track still being the one that plays, so
// that it takes the same number of frames however fast those frames come: a
// replay running ahead of the sound card would otherwise hold him far longer.
static bool M_HasSpeechEnded(M_PRIV *const p)
{
    const MUSIC_ID track = (MUSIC_ID)M_GetTrack(p->waypoint);
    if (Music_GetCurrentPlayingTrack() == track) {
        p->cut_heard = true;
        IGNORE(Music_SetSpeed(Clock_GetSpeedMultiplier()));
    }

    if (p->cut_length > 0) {
        return ++p->cut_timer >= p->cut_length;
    }
    return p->cut_heard && Music_GetCurrentPlayingTrack() != track;
}

static void M_DoCutscene(ITEM *const item, CREATURE *const info)
{
    M_PRIV *const p = M_GetPriv(item);
    ITEM *const lara = Lara_GetItem();

    const bool skip_requested =
        g_InputDB.menu_back || g_InputDB.menu_confirm || g_InputDB.option;

    if (!Lara_IsControllable()) {
        InputState_Clear(&g_Input);
        InputState_Clear(&g_InputDB);
    }

    if (skip_requested && p->cut_phase == 2) {
        p->cut_phase = 3;
        Input_HoldOffSkip();
    }

    const SECTOR *sector;
    int32_t height;
    int16_t ang, room_num;

    if (Waypoint_GetPad() != 8 && Waypoint_GetPad() != 15) {
        p->waypoint = Waypoint_GetPad();
    }

    if (Waypoint_GetPad() == 20
        && lara->current_anim_state != LS(LS_SURF_TREAD)) {
        return;
    }

    item->rot.z = 0;

    if (p->waypoint == 8 || p->waypoint == 15) {
        if (lara->current_anim_state == LS(LS_HANG)
            || lara->current_anim_state == LS(LS_SHIMMY_LEFT)
            || lara->current_anim_state == LS(LS_SHIMMY_RIGHT)) {
            Music_Play_Direct((MUSIC_ID)(M_GetTrack(p->waypoint)), MPM_ONCE);
            M_SetCutPlayed(p, p->waypoint);
            p->hold = 2;
        }

        Item_Animate(item);
        return;
    }

    switch (p->cut_phase) {
    case 0:
        Output_Overlay_SetLetterbox(24.0f / 480.0f);
        Fader_InitTo(&m_Fader, 1.0f, 1.0f, 0.0f);
        Lara_SetControllable(false);
        p->cut_phase++;
        g_Input = (INPUT_STATE) {};

        if (p->waypoint == 14) {
            Item_SwitchToAnim(item, M_ANIM_HANGING, 0);
            item->current_anim_state = M_STATE_HANGING;
            item->goal_anim_state = M_STATE_CLIMB_ON;
        } else {
            Item_SwitchToAnim(item, M_ANIM_STAND, 0);
            item->current_anim_state = M_STATE_STOP;
            item->goal_anim_state = M_STATE_STOP;
        }

        M_GetAIEnemy(info, Waypoint_GetPad());
        item->pos.x = info->enemy->pos.x;
        item->pos.y = info->enemy->pos.y;
        item->pos.z = info->enemy->pos.z;
        ang = (int16_t)Math_Atan(
            lara->pos.z - item->pos.z, lara->pos.x - item->pos.x);

        if (p->waypoint == 14 || p->waypoint == 3) {
            item->rot.y = info->enemy->rot.y;
            info->lot.is_jumping = true;
            info->maximum_turn = 0;
        } else if (p->waypoint == 43 || p->waypoint == 53) {
            info->maximum_turn = 0;
            item->rot.y = -0x6000;
        } else {
            item->rot.y = ang;
        }
        const int16_t probe_room = Room_GetIndexFromPos((XYZ_32) {
            .x = item->pos.x, .y = item->pos.y - 64, .z = item->pos.z });

        if (probe_room != item->room_num && probe_room != NO_ROOM) {
            Item_UpdateRoom(Item_GetIndex(item), probe_room);
        }

        lara->rot.y = ang + 0x8000;

        if (lara->current_anim_state != LS(LS_SURF_TREAD)) {
            room_num = lara->room_num;
            sector = Room_GetSector(lara->pos, &room_num);
            height = Room_GetHeight(sector, lara->pos);
            lara->pos.y = height;
            Item_SwitchToAnim(lara, LA(LA_STAND_STILL), 0);
            lara->current_anim_state = LS(LS_STOP);
            lara->goal_anim_state = LS(LS_STOP);
            lara->speed = 0;
            lara->fall_speed = 0;
            lara->gravity = 0;
        }

        break;

    case 1:
        if (M_IsFadedOut()) {
            M_SetCutCamera(item);
            Fader_InitTo(&m_Fader, 1.0f, 0.0f, 16.0f / (float)LOGIC_FPS);
            p->cut_phase++;
            const MUSIC_ID track = (MUSIC_ID)M_GetTrack(p->waypoint);
            Music_Play_Direct(track, MPM_ONCE);
            const double length = Music_GetTrackDuration(track);
            p->cut_length = length > 0.0 ? (int32_t)(length * LOGIC_FPS) : 0;
            p->cut_timer = 0;
        }

        break;

    case 2:
        if (g_Input.look && p->waypoint != 43 && p->waypoint != 53) {
            p->cut_phase = 3;

            if (p->waypoint != 14) {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else {
            if (M_HasSpeechEnded(p)) {
                p->cut_phase = 3;
            }

            if (item->current_anim_state == M_STATE_STOP) {
                if (info->enemy
                    && Creature_GetAIObjectFlags(info->enemy) == 36) {
                    item->goal_anim_state = M_STATE_READ_BOOK;
                } else {
                    item->goal_anim_state =
                        M_STATE_TALK_1 + (Random_GetControl() % 3);
                }
            } else if (item->current_anim_state != M_STATE_HANGING) {
                if (info->enemy
                    && Creature_GetAIObjectFlags(info->enemy) == 36) {
                    Creature_SetAIObjectSpent(info->enemy);
                }

                item->goal_anim_state = M_STATE_STOP;
            }
        }

        break;

    case 3:
        Output_Overlay_SetLetterbox(0.0f);
        m_Fader = (FADER) {};
        M_ClearCutCamera();
        const MUSIC_ID ambient = Music_GetCurrentLoopedTrack();
        Music_Stop();
        Music_Play_Direct(ambient, MPM_LOOP);
        Lara_SetControllable(true);
        p->swap_bits &= ~M_SWAP_BOOK;
        p->cut_phase = 0;
        M_SetCutPlayed(p, p->waypoint);
        ang = info->enemy->rot.y - item->rot.y;

        if (ang > 1024) {
            item->required_anim_state = M_STATE_TURN_LEFT;
        } else if (ang < -1024) {
            item->required_anim_state = M_STATE_TURN_RIGHT;
        }

        item->goal_anim_state = M_STATE_STOP;
        p->hold = 2;
        break;
    };

    Output_Overlay_SetFade(Fader_GetCurrentValue(&m_Fader));
    Item_Animate(item);

    if (item->current_anim_state == M_STATE_READ_BOOK) {
        if (Item_TestFrameEqual(item, 32)) {
            p->swap_bits |= M_SWAP_BOOK;
        } else if (Item_TestFrameEqual(item, 216)) {
            p->swap_bits &= ~M_SWAP_BOOK;
        }
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    *p = (M_PRIV) { .guides_lara = p->guides_lara };
    p->waypoint = M_WAYPOINT_UNSET;
    p->cut_phase = (int16_t)M_GetOCB(item);
    p->cut_heard = false;
    Creature_Initialise(item_num);
    Item_SwitchToAnim(item, M_ANIM_GET_KNIFE, 0);
    item->current_anim_state = M_STATE_GET_KNIFE;
    item->goal_anim_state = M_STATE_GET_KNIFE;
    p->swap_bits = M_SWAP_MESHES;
}

// The path ahead, sampled a sector at a time along his facing. He jumps a gap
// that reads as one sector across, and takes the long jump where it reads as
// two.
static void M_ProbeAhead(
    const ITEM *const item, bool *const jump_ahead, bool *const long_jump_ahead)
{
    const int32_t y = item->pos.y;
    XYZ_32 pos = item->pos;
    int32_t heights[3];
    for (int32_t i = 0; i < 3; i++) {
        pos = XYZ_32_OffsetYaw(pos, item->rot.y, M_STEP_AHEAD);
        pos.y = y;
        int16_t room_num = item->room_num;
        const SECTOR *const sector = Room_GetSector(pos, &room_num);
        heights[i] = Room_GetHeight(sector, pos);
    }

    *jump_ahead =
        y < heights[0] - 384 && y < heights[1] + 256 && y > heights[1] - 256;
    *long_jump_ahead = y < heights[0] - 384 && y < heights[1] - 384
        && y < heights[2] + 256 && y > heights[2] - 256;
}

static void M_RaceControl(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    M_PRIV *const p = M_GetPriv(item);
    const ITEM *const lara = Lara_GetItem();

    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;
    int16_t advance = 0;

    bool jump_ahead;
    bool long_jump_ahead;
    M_ProbeAhead(item, &jump_ahead, &long_jump_ahead);

    Creature_GetAITarget(creature);
    // TR4's guides pick their marker by the OCB rather than by the tag the
    // generic search matches on, so the pick is made again here.
    if ((item->ai_bits & AI_FOLLOW) != 0) {
        Creature_FindAITargetObject(creature, O_AI_FOLLOW, p->waypoint);
    }

    ITEM *const enemy = creature->enemy;

    AI_INFO info;
    if (Item_TestAnimEqual(item, M_ANIM_DROP_8_CLICKS)
        || Item_TestAnimEqual(item, M_ANIM_GRAB_CLIMB_ON)) {
        const XYZ_32 old_pos = item->pos;
        const int16_t room_num = item->room_num;
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, M_STEP_AHEAD);
        Room_GetSector(item->pos, &item->room_num);

        if (Item_TestFrameEqual(item, 1)) {
            LOT_CreateZone(item);
        }

        Creature_AIInfo(item, &info);
        item->room_num = room_num;
        item->pos = old_pos;
    } else {
        Creature_AIInfo(item, &info);
    }

    Creature_ApplyMood(item, &info, 1);
    Creature_Mood(item, &info, 1);

    int32_t lara_angle;
    int32_t distance;
    bool ahead = false;
    if (creature->enemy == lara) {
        lara_angle = info.angle;
        distance = info.distance;
    } else {
        const int32_t dx = lara->pos.x - item->pos.x;
        const int32_t dz = lara->pos.z - item->pos.z;
        lara_angle = Math_Atan(dz, dx) - item->rot.y;
        ahead = lara_angle > -DEG_90 && lara_angle < DEG_90;
        distance = SQUARE(dx) + SQUARE(dz);
    }

    angle = Creature_Turn(item, creature->maximum_turn);

    if (FlybyMode_IsActive()
        && Music_GetCurrentPlayingTrack() == M_FLYBY_TALK_TRACK) {
        IGNORE(Music_SetSpeed(Clock_GetSpeedMultiplier()));
        m_TalkTimer++;

        if ((m_TalkTimer > 0 && m_TalkTimer < 565)
            || (m_TalkTimer > 705 && m_TalkTimer < 927)) {
            const OBJECT_ID head_id =
                O_ACTOR_1_SPEECH_HEAD_1 + (Random_GetControl() & 1);
            if (m_SwappedHead != head_id) {
                if (m_SwappedHead != NO_OBJECT) {
                    Object_SwapMesh(O_VON_CROY, m_SwappedHead, M_HEAD_MESH);
                }
                Object_SwapMesh(O_VON_CROY, head_id, M_HEAD_MESH);
                m_SwappedHead = head_id;
            }
        } else {
            if (m_SwappedHead != NO_OBJECT) {
                Object_SwapMesh(O_VON_CROY, m_SwappedHead, M_HEAD_MESH);
                m_SwappedHead = NO_OBJECT;
            }
        }

        if (m_TalkTimer > 580 && m_TalkTimer < 693) {
            Lara_Skin_SetSpeechFace(Random_GetControl() & 3);
        } else {
            Lara_Skin_SetSpeechFace(-1);
        }
    } else {
        m_TalkTimer = 0;
        Lara_Skin_SetSpeechFace(-1);
        if (m_SwappedHead != NO_OBJECT) {
            Object_SwapMesh(O_VON_CROY, m_SwappedHead, M_HEAD_MESH);
            m_SwappedHead = NO_OBJECT;
        }
    }

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->lot.is_jumping = false;
        creature->lot.is_monkeying = false;
        creature->flags = 0;
        creature->maximum_turn = 0;
        head = info.angle >> 1;

        if (info.ahead) {
            torso_x = info.x_angle >> 1;
            torso_y = info.angle >> 1;
        }

        if (Waypoint_Get() < p->waypoint
            // DIVERGENCE: the original also tests the audio stream's own
            // state here (XAFlag 5 or 6), which nothing in TRX reports.
            || Music_GetCurrentPlayingTrack() == 80) {
            item->goal_anim_state = M_STATE_STOP;
            break;
        }

        if (creature->reached_goal) {
            if ((enemy != nullptr && Creature_GetAIObjectFlags(enemy) != 0
                 && (distance < M_RANGE_RUN || !Object_Get(O_BAT)->loaded))
                || Waypoint_Get() > p->waypoint) {
                if (p->hold != 6) {
                    p->hold = 0;
                }

                switch (Creature_GetAIObjectFlags(enemy)) {
                case 0:
                case 32:
                    advance = -1;
                    break;

                case 2:
                    item->current_anim_state = M_STATE_JUMP_TO_HANG;
                    Item_SwitchToAnim(item, M_ANIM_JUMP_TO_HANG, 0);
                    item->pos = enemy->pos;
                    item->rot = enemy->rot;
                    advance = 1;
                    break;

                case 4:
                    item->current_anim_state = M_STATE_DROP_8_CLICKS;
                    Item_SwitchToAnim(item, M_ANIM_DROP_8_CLICKS, 0);
                    creature->lot.is_jumping = true;
                    item->pos = enemy->pos;
                    item->rot = enemy->rot;
                    advance = 1;
                    break;

                case 6:
                    if (Waypoint_Get() > p->waypoint) {
                        advance = 1;
                    } else if ((p->swap_bits & M_SWAP_MESHES) != 0) {
                        item->goal_anim_state = M_STATE_GET_KNIFE;
                    } else {
                        item->goal_anim_state = M_STATE_ATTACK_LOW;
                    }

                    break;

                case 8:
                    if (Waypoint_Get() > p->waypoint) {
                        advance = 1;
                    } else {
                        item->goal_anim_state = M_STATE_USE_SWITCH;
                    }

                    break;

                case 10:
                    if (Waypoint_Get() > p->waypoint) {
                        advance = 1;
                    } else {
                        item->goal_anim_state = M_STATE_CHECK_GROUND;
                    }

                    break;

                case 34:
                    if (Waypoint_Get() > p->waypoint) {
                        advance = 2;
                    } else {
                        item->goal_anim_state = M_STATE_RAISED_ARM;
                    }

                    break;

                case 36:
                    if (Waypoint_Get() > p->waypoint) {
                        advance = 1;
                    } else {
                        item->goal_anim_state = M_STATE_READ_BOOK;
                    }

                    break;

                case 40:
                    if (p->hold == 6) {
                        item->goal_anim_state = M_STATE_RUN;
                    } else {
                        item->goal_anim_state = M_STATE_HOP_BACK;
                        item->pos = enemy->pos;
                        item->rot = enemy->rot;
                    }

                    break;
                }
            } else {
                advance = 1;
            }
        } else if (jump_ahead || long_jump_ahead) {
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_JUMP_FORWARD_START, 0);
            item->current_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;

            if (long_jump_ahead) {
                item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
            } else {
                item->goal_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;
            }

            creature->lot.is_jumping = true;
        } else if (creature->monkey_ahead) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            const int32_t height = Room_GetHeight(sector, item->pos);
            const int32_t ceiling = Room_GetCeiling(sector, item->pos);

            if (ceiling == height - WALL_L * 3 / 2) {
                item->goal_anim_state = M_STATE_MONKEY_IDLE;
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
        } else if (enemy != lara || info.distance > M_RANGE_WALK) {
            item->goal_anim_state = M_STATE_WALK;
        }

        break;

    case M_STATE_WALK:
        creature->lot.is_jumping = false;
        creature->lot.is_monkeying = false;
        creature->maximum_turn = 1092;

        if (ahead) {
            head = (int16_t)lara_angle;
        } else if (info.ahead) {
            head = info.angle;
        }

        // The original starts the level timer here; TRX runs it from the
        // level itself, so there is nothing to start.

        if (Waypoint_Get() < p->waypoint) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (jump_ahead || long_jump_ahead) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->monkey_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (!creature->reached_goal) {
            if (info.distance < M_RANGE_WALK
                && Creature_GetAIObjectFlags(enemy) != 32) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.distance > M_RANGE_RUN) {
                item->goal_anim_state = M_STATE_RUN;
            }
        } else if (Creature_GetAIObjectFlags(enemy) == 32) {
            advance = -1;
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }

        break;

    case M_STATE_RUN:
        if (info.ahead) {
            head = info.angle;
        }

        if (Item_TestFrameEqual(item, 0)) {
            creature->lot.is_jumping = false;
            creature->maximum_turn = 1456;
        }

        tilt = angle >> 1;

        if (p->hold == 6) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
        } else if (Waypoint_Get() < p->waypoint || jump_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (long_jump_ahead) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
        } else if (creature->monkey_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->reached_goal) {
            if (Creature_GetAIObjectFlags(enemy) == 32) {
                advance = -1;
            } else if (info.distance >= M_RANGE_MARKER) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_GetAIObjectFlags(enemy) == 40) {
                creature->maximum_turn = 0;
                item->rot.y = enemy->rot.y;
                item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
                p->hold = 6;
            }
        } else if (
            info.distance < M_RANGE_WALK
            && Creature_GetAIObjectFlags(enemy) != 32
            && Creature_GetAIObjectFlags(enemy) != 40) {
            item->goal_anim_state = M_STATE_STOP;
        }

        break;

    case M_STATE_MONKEY_IDLE:
        creature->maximum_turn = 0;

        if (item->box_num == creature->lot.target_box
            || !creature->monkey_ahead) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            const int32_t height = Room_GetHeight(sector, item->pos);
            const int32_t ceiling = Room_GetCeiling(sector, item->pos);

            if (ceiling == height - WALL_L * 3 / 2) {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else {
            item->goal_anim_state = M_STATE_MONKEY_FORWARD;
        }

        break;

    case M_STATE_MONKEY_FORWARD:
        creature->lot.is_jumping = true;
        creature->lot.is_monkeying = true;
        creature->maximum_turn = 1092;

        if (item->box_num == creature->lot.target_box
            || !creature->monkey_ahead) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            const int32_t height = Room_GetHeight(sector, item->pos);
            const int32_t ceiling = Room_GetCeiling(sector, item->pos);

            if (ceiling == height - WALL_L * 3 / 2) {
                item->goal_anim_state = M_STATE_MONKEY_IDLE;
            }
        }

        break;

    case M_STATE_GET_KNIFE:
        if (Item_TestFrameEqual(item, 28)) {
            if ((p->swap_bits & M_SWAP_MESHES) != 0) {
                p->swap_bits &= ~M_SWAP_MESHES;
            } else {
                p->swap_bits |= M_SWAP_MESHES;
            }
        }

        break;

    case M_STATE_JUMP_FORWARD_1_BLOCK:
        if (Item_TestAnimEqual(item, M_ANIM_JUMP_FORWARD_1_BLOCK)) {
            item->goal_anim_state = M_STATE_RUN;
        }

        break;

    case M_STATE_JUMP_FORWARD_2_BLOCK:
        if (Item_TestAnimEqual(item, M_ANIM_JUMP_FORWARD_2_BLOCK)
            || Item_GetRelativeFrame(item) > 7) {
            creature->lot.is_jumping = true;
        } else if (jump_ahead) {
            item->goal_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;
        } else if (!Object_Get(O_BAT)->loaded) {
            item->goal_anim_state = M_STATE_RUN;
        }

        if (p->hold == 6) {
            item->goal_anim_state = M_STATE_GRAB_CLIMB_ON;
        }

        break;

    case M_STATE_USE_SWITCH:
        if (Item_TestFrameEqual(item, 0)) {
            item->pos = enemy->pos;
            item->rot = enemy->rot;
        } else if (Item_TestFrameEqual(item, 120)) {
            advance = -1;
        }

        break;

    case M_STATE_TURN_LEFT:
    case M_STATE_TURN_RIGHT:
        creature->maximum_turn = 0;

        if (p->hold != 0) {
            Creature_TurnTo(item, enemy->rot.y - item->rot.y, 512);
        } else {
            Creature_TurnTo(item, (int16_t)lara_angle, 512);
        }

        break;

    case M_STATE_ATTACK_LOW:
        if (info.ahead) {
            head = info.angle >> 1;
            torso_y = info.angle >> 1;
            torso_x = info.x_angle >> 1;
        }

        creature->maximum_turn = 0;

        if (ABS(info.angle) < 1092) {
            item->rot.y += info.angle;
        } else if (info.angle < 0) {
            item->rot.y -= 1092;
        } else {
            item->rot.y += 1092;
        }

        if (enemy != nullptr && Creature_GetAIObjectFlags(enemy) == 6
            && Item_GetRelativeFrame(item) > 21) {
            advance = -1;
        } else if (creature->flags == 0 && enemy != nullptr) {
            if (Item_TestFrameRange(item, 16, 25)) {
                if (ABS(enemy->pos.x - item->pos.x) < 512
                    && ABS(enemy->pos.y - item->pos.y) <= 512
                    && ABS(enemy->pos.z - item->pos.z) < 512) {
                    enemy->hit_points -= 20;
                    enemy->hit_status = true;
                    creature->flags = 1;
                    Creature_Effect(item, &m_Bite, Spawn_Blood);
                }
            }
        }

        break;

    case M_STATE_GRAB_CLIMB_ON:
        if (Item_TestAnimEqual(item, M_ANIM_GRAB_CLIMB_ON)
            && Item_GetRelativeFrame(item) == 0) {
            advance = 1;
        }

        item->goal_anim_state = M_STATE_WALK;
        p->hold = 0;
        break;

    case M_STATE_HOP_BACK:
        p->hold = 6;
        break;

    case M_STATE_CORRECT_POSITION_FRONT:
    case M_STATE_CORRECT_POSITION_BACK:
        creature->maximum_turn = 0;
        Creature_MoveTo(item, enemy, 8, enemy->rot.y - item->rot.y, 512);
        break;
    }

    if (advance == -1 && enemy != nullptr) {
        Room_TestTriggersEx(enemy, true);
        advance = 1;
    }

    if (advance != 0) {
        creature->reached_goal = false;
        creature->enemy = nullptr;
        p->waypoint += advance;
        item->ai_bits = AI_FOLLOW;
    }

    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);
    Creature_Joint(item, 3, torso_x);

    if (item->current_anim_state >= M_STATE_JUMP_FORWARD_1_BLOCK
        || item->current_anim_state == M_STATE_MONKEY_FORWARD) {
        Creature_Animate(item_num, angle, 0);
    } else {
        switch (Creature_Vault(item_num, angle, 2, 260)) {
        case -4:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_DROP_4_CLICKS, 0);
            item->current_anim_state = M_STATE_DROP_4_CLICKS;
            break;

        case -3:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_DROP_3_CLICKS, 0);
            item->current_anim_state = M_STATE_DROP_3_CLICKS;
            break;

        case -2:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_DROP_2_CLICKS, 0);
            item->current_anim_state = M_STATE_DROP_2_CLICKS;
            break;

        case 2:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_VAULT_2_CLICKS, 0);
            item->current_anim_state = M_STATE_VAULT_2_CLICKS;
            break;

        case 3:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_VAULT_3_CLICKS, 0);
            item->current_anim_state = M_STATE_VAULT_3_CLICKS;
            break;

        case 4:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_VAULT_4_CLICKS, 0);
            item->current_anim_state = M_STATE_VAULT_4_CLICKS;
            break;
        }
    }
}

static void M_GuideControl(const int16_t item_num)
{
    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    CREATURE *const creature = item->creature_data;
    M_PRIV *const p = M_GetPriv(item);
    const ITEM *const lara = Lara_GetItem();

    int16_t tilt = 0;
    int16_t angle = 0;
    int16_t head = 0;
    int16_t torso_x = 0;
    int16_t torso_y = 0;
    int16_t advance = 0;

    bool jump_ahead;
    bool long_jump_ahead;
    M_ProbeAhead(item, &jump_ahead, &long_jump_ahead);

    item->ai_bits = AI_FOLLOW;
    Creature_GetAITarget(creature);
    // TR4's guides pick their marker by the OCB rather than by the tag the
    // generic search matches on, so the pick is made again here.
    if ((item->ai_bits & AI_FOLLOW) != 0) {
        Creature_FindAITargetObject(creature, O_AI_FOLLOW, p->waypoint);
    }
    ITEM *target = nullptr;

    if (Waypoint_Get() <= p->waypoint) {
        int32_t max_dist = 0x7FFFFFFF;

        for (int32_t i = 0; i < 5; i++) {
            const CREATURE *const baddie = LOT_GetBaddieSlot(i);

            if (baddie->item_num == NO_ITEM || baddie->item_num == item_num) {
                continue;
            }

            ITEM *const candidate = Item_Get(baddie->item_num);

            if (candidate->object_id != O_VON_CROY) {
                const int32_t dx = candidate->pos.x - item->pos.x;
                const int32_t dz = candidate->pos.z - item->pos.z;
                const int32_t dist = SQUARE(dx) + SQUARE(dz);

                if (ABS(dx) <= WALL_L * 5 && ABS(dz) <= WALL_L * 5
                    && dist < max_dist) {
                    creature->reached_goal = false;
                    target = candidate;
                    max_dist = dist;
                    p->hold = 0;
                }
            }
        }
    }

    ITEM *enemy = creature->enemy;

    if (target != nullptr) {
        creature->enemy = target;
    }

    if (Item_TestAnimEqual(item, M_ANIM_DROP_8_CLICKS)
        || Item_TestAnimEqual(item, M_ANIM_GRAB_CLIMB_ON)) {
        const XYZ_32 old_pos = item->pos;
        const int16_t room_num = item->room_num;
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, M_STEP_AHEAD);
        Room_GetSector(item->pos, &item->room_num);

        if (Item_TestFrameEqual(item, 1)) {
            LOT_CreateZone(item);
        }

        Creature_AIInfo(item, &m_AI);
        item->room_num = room_num;
        item->pos = old_pos;
    } else {
        Creature_AIInfo(item, &m_AI);
    }

    Creature_ApplyMood(item, &m_AI, 1);
    Creature_Mood(item, &m_AI, 1);

    if (creature->enemy == lara) {
        m_LaraAI = m_AI;
    } else {
        int32_t dx = lara->pos.x - item->pos.x;
        int32_t dz = lara->pos.z - item->pos.z;
        m_LaraAI.angle = (int16_t)(Math_Atan(dz, dx) - item->rot.y);
        m_LaraAI.ahead = m_LaraAI.angle > -DEG_90 && m_LaraAI.angle < DEG_90;
        m_LaraAI.enemy_facing = m_LaraAI.angle + 0x8000 - lara->rot.y;

        if (dx > 32000 || dx < -32000 || dz > 32000 || dz < -32000) {
            m_LaraAI.distance = 0x7FFFFFFF;
        } else {
            m_LaraAI.distance = SQUARE(dx) + SQUARE(dz);
        }

        dx = ABS(dx);
        dz = ABS(dz);

        if (dx > dz) {
            m_LaraAI.x_angle =
                (int16_t)Math_Atan(dx + (dz >> 1), item->pos.y - lara->pos.y);
        } else {
            m_LaraAI.x_angle =
                (int16_t)Math_Atan(dz + (dx >> 1), item->pos.y - lara->pos.y);
        }
    }

    m_LaraAI.bite = m_LaraAI.angle > -0x1800 && m_LaraAI.angle < 0x1800
        && m_LaraAI.distance < M_RANGE_ATTACK;
    angle = Creature_Turn(item, creature->maximum_turn);

    if (target != nullptr) {
        creature->enemy = enemy;
        enemy = target;
    }

    if (p->waypoint == 43 && Stats_CheckAllLevelSecretsPickedUp()) {
        creature->reached_goal = false;
        creature->enemy = nullptr;
        item->ai_bits = AI_FOLLOW;
        p->waypoint = 53;
        Waypoint_Set(53);
    }

    if ((Waypoint_GetPad() == 9 || Waypoint_GetPad() == 10)
        && p->waypoint == 11) {
        Waypoint_SetPad(11);
    } else if (
        Waypoint_GetPad() == 10 && p->waypoint == 12
        && (p->climb_done
            || (Item_TestAnimEqual(lara, LA(LA_SMALL_JUMP_BACK_END))
                && Item_TestFrameEqual(lara, -1)))) {
        Waypoint_SetPad((int16_t)p->waypoint);
        p->climb_done = true;
    } else if (
        Waypoint_GetPad() == 43 && (p->waypoint == 43 || p->waypoint == 53)) {
        Waypoint_SetPad((int16_t)p->waypoint);
        Waypoint_Set(Waypoint_GetPad());
    } else if (
        Waypoint_Get() == 43
        && (p->waypoint == 44 || p->waypoint == 54 || p->waypoint == 44
            || p->waypoint == 54)) {
        Waypoint_Set((int16_t)p->waypoint);
    }

    if (!M_IsCutPlayed(p, p->waypoint)) {
        if ((creature->reached_goal && p->waypoint == Waypoint_GetPad()
             && M_GetTrack(p->waypoint) != -1)
            || p->cut_phase > 0
            || (Waypoint_GetPad() >= p->waypoint
                && !M_IsCutPlayed(p, Waypoint_GetPad())
                && M_GetTrack(Waypoint_GetPad()) != -1)) {
            Creature_Joint(item, 0, m_LaraAI.angle >> 1);
            Creature_Joint(item, 1, m_LaraAI.x_angle >> 1);
            Creature_Joint(item, 2, m_LaraAI.angle >> 1);
            Creature_Joint(item, 3, m_LaraAI.x_angle >> 1);
            M_DoCutscene(item, creature);
            return;
        }
    }

    switch (item->current_anim_state) {
    case M_STATE_STOP:
        creature->lot.is_jumping = false;
        creature->lot.is_monkeying = false;
        creature->flags = 0;
        creature->maximum_turn = 0;
        head = m_AI.angle >> 1;

        if (m_AI.ahead) {
            torso_x = m_AI.x_angle >> 1;
            torso_y = m_AI.angle >> 1;
        }

        if (item->required_anim_state != M_STATE_NULL) {
            item->goal_anim_state = item->required_anim_state;
        } else if (p->hold == 2) {
            if (enemy->rot.y - item->rot.y < -1024) {
                item->goal_anim_state = M_STATE_TURN_RIGHT;
            } else if (enemy->rot.y - item->rot.y > 1024) {
                item->goal_anim_state = M_STATE_TURN_LEFT;
            } else {
                p->hold = 0;

                if (Creature_GetAIObjectFlags(enemy) == 0) {
                    advance = 1;
                }
            }
        } else if (Waypoint_Get() < p->waypoint && creature->reached_goal) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (
            target != nullptr && m_AI.distance < M_RANGE_RUN
            && (p->swap_bits & M_SWAP_MESHES) != 0) {
            item->goal_anim_state = M_STATE_GET_KNIFE;
        } else if (target != nullptr && m_AI.distance < M_RANGE_ATTACK) {
            if (m_AI.bite) {
                item->goal_anim_state = M_STATE_ATTACK_LOW;
            } else if (enemy->hit_points > 0 && m_AI.ahead) {
                if (ABS(enemy->pos.y + 512 - item->pos.y) < 512) {
                    item->goal_anim_state = M_STATE_ATTACK_HIGH;
                }
            }
        } else if (
            target != nullptr && enemy != lara
            && m_AI.distance > M_RANGE_WALK) {
            item->goal_anim_state = M_STATE_WALK;
        } else if (creature->reached_goal) {
            if (m_AI.distance > M_RANGE_MARKER
                && Creature_GetAIObjectFlags(enemy) != 0 && p->hold != 6) {
                creature->maximum_turn = 0;

                if (m_AI.ahead) {
                    item->required_anim_state = M_STATE_CORRECT_POSITION_FRONT;
                } else {
                    item->required_anim_state = M_STATE_CORRECT_POSITION_BACK;
                }

                break;
            }

            if (Waypoint_Get() > p->waypoint
                || (enemy != nullptr && Creature_GetAIObjectFlags(enemy) != 0
                    && (Waypoint_GetPad() == p->waypoint
                        || (M_GetTrack(p->waypoint) == -1
                            && Waypoint_Get() == p->waypoint
                            && m_LaraAI.distance < M_RANGE_RUN)))) {
                if (Creature_GetAIObjectFlags(enemy) > 32) {
                    switch (Creature_GetAIObjectFlags(enemy)) {
                    case 34:
                        if (Waypoint_Get() > p->waypoint) {
                            advance = 2;
                        } else {
                            item->goal_anim_state = M_STATE_RAISED_ARM;
                        }

                        break;

                    case 36:
                        if (Waypoint_Get() > p->waypoint) {
                            advance = 1;
                        } else {
                            item->goal_anim_state = M_STATE_STOP;
                        }

                        break;

                    case 40:
                        if (p->hold == 6) {
                            item->goal_anim_state = M_STATE_RUN;
                        } else {
                            item->goal_anim_state = M_STATE_HOP_BACK;
                            item->pos = enemy->pos;
                            item->rot = enemy->rot;
                        }

                        break;

                    case 48:
                        advance = -1;
                        break;

                    case AI_OBJECT_FLAGS_SPENT:
                        advance = 1;
                        break;
                    }
                } else if (Creature_GetAIObjectFlags(enemy) == 32) {
                    advance = -1;
                } else {
                    switch (Creature_GetAIObjectFlags(enemy)) {
                    case 0:
                        advance = -1;
                        break;

                    case 2:
                        Item_SwitchToAnim(item, M_ANIM_JUMP_TO_HANG, 0);
                        item->current_anim_state = M_STATE_JUMP_TO_HANG;
                        item->pos = enemy->pos;
                        item->rot = enemy->rot;
                        advance = 1;
                        break;

                    case 4:
                        Item_SwitchToAnim(item, M_ANIM_DROP_8_CLICKS, 0);
                        item->current_anim_state = M_STATE_DROP_8_CLICKS;
                        creature->lot.is_jumping = true;
                        item->pos = enemy->pos;
                        item->rot = enemy->rot;
                        advance = 1;
                        break;

                    case 8:
                        item->goal_anim_state = M_STATE_USE_SWITCH;
                        break;

                    case 10:
                        item->goal_anim_state = M_STATE_CHECK_GROUND;
                        break;

                    case 12:
                        creature->maximum_turn = 0;
                        Item_SwitchToAnim(item, M_ANIM_JUMP_FORWARD_START, 0);
                        item->current_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;

                        if (long_jump_ahead) {
                            item->goal_anim_state =
                                M_STATE_JUMP_FORWARD_2_BLOCK;
                        } else {
                            item->goal_anim_state =
                                M_STATE_JUMP_FORWARD_1_BLOCK;
                        }

                        creature->lot.is_jumping = true;
                        item->pos = enemy->pos;
                        item->rot = enemy->rot;
                        advance = 1;
                        break;
                    }
                }
            } else if (
                enemy && Creature_GetAIObjectFlags(enemy)
                && m_LaraAI.distance >= M_RANGE_RUN) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (p->hold != 0) {
                if (p->hold != 1) {
                    advance = 1;
                } else if ((Random_GetControl() & 0xF) != 0) {
                    p->hold = 0;
                } else if (m_LaraAI.distance >= M_RANGE_RUN) {
                    item->goal_anim_state = M_STATE_USHER_FAR;
                } else {
                    item->goal_anim_state = M_STATE_USHER_NEAR;
                }
            } else if (m_LaraAI.angle > 1024) {
                item->goal_anim_state = M_STATE_TURN_RIGHT;
            } else if (m_LaraAI.angle < -1024) {
                item->goal_anim_state = M_STATE_TURN_LEFT;
            } else {
                p->hold = 1;
            }
        } else if (m_LaraAI.bite) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->monkey_ahead) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            const int32_t height = Room_GetHeight(sector, item->pos);
            const int32_t ceiling = Room_GetCeiling(sector, item->pos);

            if (ceiling == height - WALL_L * 3 / 2) {
                if ((p->swap_bits & M_SWAP_MESHES) != 0) {
                    item->goal_anim_state = M_STATE_MONKEY_IDLE;
                } else {
                    item->goal_anim_state = M_STATE_GET_KNIFE;
                }
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
        } else if (
            target != nullptr && m_AI.distance < M_RANGE_RUN
            && (p->swap_bits & M_SWAP_MESHES) != 0) {
            item->goal_anim_state = M_STATE_GET_KNIFE;
        } else if (target != nullptr && m_AI.distance < M_RANGE_ATTACK) {
            if (m_AI.bite) {
                item->goal_anim_state = M_STATE_ATTACK_LOW;
            } else if (enemy->hit_points > 0 && m_AI.ahead) {
                if (ABS(enemy->pos.y + 512 - item->pos.y) < 512) {
                    item->goal_anim_state = M_STATE_ATTACK_HIGH;
                }
            }
        } else if (
            (m_AI.distance > M_RANGE_WALK && m_LaraAI.distance < M_RANGE_LOST)
            || Waypoint_Get() >= p->waypoint) {
            item->goal_anim_state = M_STATE_WALK;
        }

        break;

    case M_STATE_WALK:
        creature->lot.is_jumping = false;
        creature->lot.is_monkeying = false;
        creature->maximum_turn = 1092;

        if (m_LaraAI.ahead) {
            head = m_LaraAI.angle;
        } else if (m_AI.ahead) {
            head = m_AI.angle;
        }

        if (item->required_anim_state != M_STATE_NULL) {
            item->goal_anim_state = item->required_anim_state;
        } else if (
            (Waypoint_Get() < p->waypoint && m_LaraAI.distance > M_RANGE_LOST)
            || m_LaraAI.bite) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->monkey_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->reached_goal) {
            if (Creature_GetAIObjectFlags(enemy) == 32) {
                advance = -1;
            } else {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else if (
            target == nullptr
            || (m_AI.distance >= M_RANGE_TARGET
                && ((p->swap_bits & M_SWAP_MESHES) != 0
                    || m_AI.distance >= M_RANGE_RUN))) {
            if (m_AI.distance < M_RANGE_WALK
                && Creature_GetAIObjectFlags(enemy) != 32) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (
                m_AI.distance > M_RANGE_RUN && Waypoint_Get() >= p->waypoint) {
                item->goal_anim_state = M_STATE_RUN;
            }
        } else {
            item->goal_anim_state = M_STATE_STOP;
        }

        break;

    case M_STATE_RUN:
        if (m_AI.ahead) {
            head = m_AI.angle;
        }

        if (Item_TestFrameEqual(item, 0)) {
            creature->lot.is_jumping = false;
            creature->maximum_turn = 1456;
        }

        tilt = angle >> 1;

        if (p->hold == 6) {
            creature->maximum_turn = 0;
            item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
        } else if (
            Waypoint_Get() < p->waypoint || jump_ahead || m_LaraAI.bite) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->monkey_ahead) {
            item->goal_anim_state = M_STATE_STOP;
        } else if (creature->reached_goal) {
            if (Creature_GetAIObjectFlags(enemy) == 32) {
                advance = -1;
            } else if (m_AI.distance >= 512) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (Creature_GetAIObjectFlags(enemy) == 40) {
                creature->maximum_turn = 0;
                item->rot.y = enemy->rot.y;
                item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
                p->hold = 6;
            }
        } else if (
            m_AI.distance < M_RANGE_WALK
            && Creature_GetAIObjectFlags(enemy) != 32
            && Creature_GetAIObjectFlags(enemy) != 40) {
            item->goal_anim_state = M_STATE_STOP;
        }

        break;

    case M_STATE_MONKEY_IDLE:
        creature->maximum_turn = 0;

        if (item->box_num == creature->lot.target_box
            || !creature->monkey_ahead) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            const int32_t height = Room_GetHeight(sector, item->pos);
            const int32_t ceiling = Room_GetCeiling(sector, item->pos);

            if (ceiling == height - WALL_L * 3 / 2) {
                item->goal_anim_state = M_STATE_STOP;
            }
        } else {
            item->goal_anim_state = M_STATE_MONKEY_FORWARD;
        }

        break;

    case M_STATE_MONKEY_FORWARD:
        creature->lot.is_jumping = true;
        creature->lot.is_monkeying = true;
        creature->maximum_turn = 1092;

        if (item->box_num == creature->lot.target_box
            || !creature->monkey_ahead) {
            int16_t room_num = item->room_num;
            const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
            const int32_t height = Room_GetHeight(sector, item->pos);
            const int32_t ceiling = Room_GetCeiling(sector, item->pos);

            if (ceiling == height - WALL_L * 3 / 2) {
                item->goal_anim_state = M_STATE_MONKEY_IDLE;
            }
        }

        break;

    case M_STATE_GET_KNIFE:
        if (Item_TestFrameEqual(item, 28)) {
            if ((p->swap_bits & M_SWAP_MESHES) != 0) {
                p->swap_bits &= ~M_SWAP_MESHES;
            } else {
                p->swap_bits |= M_SWAP_MESHES;
            }
        }

        break;

    case M_STATE_CHECK_GROUND:
        if (Item_TestFrameEqual(item, 0)) {
            item->pos = enemy->pos;
            item->rot = enemy->rot;

            if (p->waypoint == 6) {
                creature->maximum_turn = 0;
                Item_SwitchToAnim(item, M_ANIM_JUMP_FORWARD_START, 0);
                item->current_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;
                item->goal_anim_state = M_STATE_JUMP_FORWARD_2_BLOCK;
                creature->lot.is_jumping = true;
            }

            advance = 1;
        }

        break;

    case M_STATE_JUMP_FORWARD_2_BLOCK:
        if (Item_TestAnimEqual(item, M_ANIM_JUMP_FORWARD_2_BLOCK)
            || Item_GetRelativeFrame(item) > 5) {
            creature->lot.is_jumping = true;
        } else if (jump_ahead) {
            item->goal_anim_state = M_STATE_JUMP_FORWARD_1_BLOCK;
        }

        if (p->hold == 6) {
            item->goal_anim_state = M_STATE_GRAB_CLIMB_ON;
        }

        break;

    case M_STATE_USE_SWITCH:
        if (Item_TestFrameEqual(item, 0)) {
            item->pos = enemy->pos;
            item->rot = enemy->rot;
        } else if (Item_TestFrameEqual(item, 120)) {
            advance = -1;
        }

        break;

    case M_STATE_ATTACK_HIGH:
        if (m_AI.ahead) {
            torso_y = m_AI.angle >> 1;
            head = m_AI.angle >> 1;
            torso_x = m_AI.x_angle >> 1;
        }

        creature->maximum_turn = 0;
        Creature_TurnTo(item, m_AI.angle, 1092);

        if (creature->flags == 0 && enemy != nullptr
            && Item_TestFrameRange(item, 21, 44)) {
            if (ABS(enemy->pos.x - item->pos.x) < 512
                && ABS(enemy->pos.y + 768 - item->pos.y) <= 512
                && ABS(enemy->pos.z - item->pos.z) < 512) {
                enemy->hit_points -= 40;

                if (enemy->hit_points <= 0) {
                    item->ai_bits = AI_FOLLOW;
                }

                enemy->hit_status = true;
                creature->flags = 1;
                Creature_Effect(item, &m_Bite, Spawn_Blood);
            }
        }

        break;

    case M_STATE_TURN_LEFT:
    case M_STATE_TURN_RIGHT:
        creature->maximum_turn = 0;

        if (p->hold != 0 && enemy != nullptr) {
            Creature_TurnTo(item, enemy->rot.y - item->rot.y, 512);
        } else {
            Creature_TurnTo(item, m_LaraAI.angle, 512);
        }

        break;

    case M_STATE_HANGING:
        creature->lot.is_jumping = true;
        creature->maximum_turn = 0;

        if (creature->reached_goal) {
            item->goal_anim_state = M_STATE_CLIMB_ON;
            advance = 1;
        } else {
            item->goal_anim_state = M_STATE_SHIMMY_RIGHT;
        }

        break;

    case M_STATE_SHIMMY_RIGHT:
        creature->lot.is_jumping = true;
        creature->maximum_turn = 0;
        break;

    case M_STATE_ATTACK_LOW:
        if (m_AI.ahead) {
            torso_y = m_AI.angle >> 1;
            head = m_AI.angle >> 1;
            torso_x = m_AI.x_angle >> 1;
        }

        creature->maximum_turn = 0;
        Creature_TurnTo(item, m_AI.angle, 1092);

        if (enemy != nullptr && Creature_GetAIObjectFlags(enemy) == 6
            && Item_GetRelativeFrame(item) > 21) {
            advance = -1;
            creature->flags = 1;
        } else if (
            creature->flags == 0 && enemy != nullptr
            && Item_TestFrameRange(item, 16, 25)) {
            if (ABS(enemy->pos.x - item->pos.x) < 512
                && ABS(enemy->pos.y - item->pos.y) <= 512
                && ABS(enemy->pos.z - item->pos.z) < 512) {
                enemy->hit_points -= 20;

                if (enemy->hit_points <= 0) {
                    item->ai_bits = AI_FOLLOW;
                }

                enemy->hit_status = true;
                creature->flags = 1;
                Creature_Effect(item, &m_Bite, Spawn_Blood);
            }
        }

        break;

    case M_STATE_RAISED_ARM:
        if (m_AI.ahead) {
            torso_y = m_AI.angle >> 1;
            head = m_AI.angle >> 1;
            torso_x = m_AI.x_angle;
        }

        creature->maximum_turn = 0;
        Creature_TurnTo(item, m_AI.angle, 1092);

        if (Item_TestAnimEqual(item, M_ANIM_RAISE_ARM)) {
            if (Item_TestFrameEqual(item, 0)) {
                advance = 1;
            }
        } else if ((Random_GetControl() & 0x1F) == 0) {
            advance = 1;
            item->goal_anim_state = M_STATE_STOP;
        }

        break;

    case M_STATE_GRAB_CLIMB_ON:
        if (Item_TestAnimEqual(item, M_ANIM_GRAB_CLIMB_ON)
            && Item_TestFrameEqual(item, 0)) {
            advance = 1;
        }

        item->goal_anim_state = M_STATE_WALK;
        item->required_anim_state = M_STATE_RUN;
        p->hold = 0;
        break;

    case M_STATE_HOP_BACK:
        p->hold = 6;
        break;

    case M_STATE_CORRECT_POSITION_FRONT:
    case M_STATE_CORRECT_POSITION_BACK:
        creature->maximum_turn = 0;
        Creature_MoveTo(item, enemy, 15, enemy->rot.y - item->rot.y, 512);
        break;
    }

    if (advance == -1) {
        enemy = creature->enemy;
        if (enemy != nullptr) {
            Room_TestTriggersEx(enemy, true);
        }
        advance = 1;
    }

    if (advance != 0) {
        creature->reached_goal = false;
        creature->enemy = nullptr;
        item->ai_bits = AI_FOLLOW;
        p->waypoint += advance;
    }

    Creature_Tilt(item, tilt);
    Creature_Joint(item, 0, torso_y);
    Creature_Joint(item, 1, torso_x);
    Creature_Joint(item, 2, head);
    Creature_Joint(item, 3, torso_x);

    if (item->current_anim_state >= M_STATE_JUMP_FORWARD_1_BLOCK
        || item->current_anim_state == M_STATE_MONKEY_FORWARD) {
        Creature_Animate(item_num, angle, 0);
    } else {
        switch (Creature_Vault(item_num, angle, 2, 260)) {
        case -4:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_DROP_4_CLICKS, 0);
            item->current_anim_state = M_STATE_DROP_4_CLICKS;
            break;

        case -3:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_DROP_3_CLICKS, 0);
            item->current_anim_state = M_STATE_DROP_3_CLICKS;
            break;

        case -2:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_DROP_2_CLICKS, 0);
            item->current_anim_state = M_STATE_DROP_2_CLICKS;
            break;

        case 2:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_VAULT_2_CLICKS, 0);
            item->current_anim_state = M_STATE_VAULT_2_CLICKS;
            break;

        case 3:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_VAULT_3_CLICKS, 0);
            item->current_anim_state = M_STATE_VAULT_3_CLICKS;
            break;

        case 4:
            creature->maximum_turn = 0;
            Item_SwitchToAnim(item, M_ANIM_VAULT_4_CLICKS, 0);
            item->current_anim_state = M_STATE_VAULT_4_CLICKS;
            break;
        }
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = M_GetPriv(item);
    // The marker names the waypoint he starts from, and which marker he
    // stands on is only known once the AI bits have been handed out, which
    // happens after every item has been initialised.
    if (p->waypoint == M_WAYPOINT_UNSET) {
        p->waypoint = item->ai_ocb;
    }

    if (p->guides_lara) {
        M_GuideControl(item_num);
    } else {
        M_RaceControl(item_num);
    }
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    M_PRIV *const p = M_GetPriv(item);
    if (stage != SAVEGAME_STAGE_AFTER_LOAD || p->cut_phase == 0) {
        return;
    }

    Output_Overlay_SetLetterbox(0.0f);
    Output_Overlay_SetFade(0.0f);
    m_Fader = (FADER) {};
    Lara_SetControllable(true);
    M_SetCutPlayed(p, p->waypoint);
    p->cut_phase = 0;
    p->cut_heard = false;
    p->cut_length = 0;
    p->cut_timer = 0;
    p->hold = 2;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    SHOULD(JSON_READ(io, "swap_bits", &p->swap_bits));
    SHOULD(JSON_READ(io, "waypoint", &p->waypoint));
    SHOULD(JSON_READ(io, "hold", &p->hold));
    SHOULD(JSON_READ(io, "climb_done", &p->climb_done));
    SHOULD(JSON_READ(io, "cut_phase", &p->cut_phase));
    SHOULD(JSON_READ(io, "cut_heard", &p->cut_heard));
    SHOULD(JSON_READ(io, "cut_played", &p->cut_played));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "swap_bits", p->swap_bits);
    JSONW_WRITE(io, "waypoint", p->waypoint);
    JSONW_WRITE(io, "hold", p->hold);
    JSONW_WRITE(io, "climb_done", p->climb_done);
    JSONW_WRITE(io, "cut_phase", p->cut_phase);
    JSONW_WRITE(io, "cut_heard", p->cut_heard);
    JSONW_WRITE(io, "cut_played", p->cut_played);
}

static bool M_CanTakeDamage(const ITEM *const item)
{
    return false;
}

// The mask names the meshes drawn from his own object, the other way round
// from swap_bits. His climbs hang the mesh well below the item, outside the
// clip window of the room the item is still in, so he draws against the whole
// viewport the way doors and switches do.
static bool M_Draw(const ITEM *const item)
{
    const M_PRIV *const p = M_GetPriv(item);
    const OBJECT *const swap = Object_Get(O_MESH_SWAP_1);
    if (!swap->loaded
        || swap->mesh_count != Object_Get(item->object_id)->mesh_count) {
        return Object_DrawUnclippedItem(item);
    }

    const int32_t left = g_PhdLeft;
    const int32_t top = g_PhdTop;
    const int32_t right = g_PhdRight;
    const int32_t bottom = g_PhdBottom;

    g_PhdLeft = Viewport_GetMinX(VIEWPORT_GAME);
    g_PhdTop = Viewport_GetMinY(VIEWPORT_GAME);
    g_PhdRight = Viewport_GetMaxX(VIEWPORT_GAME);
    g_PhdBottom = Viewport_GetMaxY(VIEWPORT_GAME);

    const bool result = Object_DrawAnimatingItemWithSwap(
        item, swap, item->mesh_bits & ~p->swap_bits);

    g_PhdLeft = left;
    g_PhdTop = top;
    g_PhdRight = right;
    g_PhdBottom = bottom;
    return result;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->control_func = M_Control;
    obj->draw_func = M_Draw;
    obj->collision_func = Creature_Collision;
    obj->initialise_func = M_Initialise;
    obj->can_take_damage_func = M_CanTakeDamage;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->handle_save_func = M_HandleSave;

    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;
    obj->radius = M_RADIUS;
    obj->smartness = 0x7FFF;
    obj->intelligent = true;
    obj->lot_setup = LOT_Setup(LOT_SETUP_ACROBAT);

    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_anim = true;
    obj->save_flags = true;

    Object_GetBone(obj, 6)->rot.y = true;
    Object_GetBone(obj, 6)->rot.x = true;
    Object_GetBone(obj, 20)->rot.y = true;
    Object_GetBone(obj, 20)->rot.x = true;
    OBJECT_PROPERTIES(
        obj, ITEM_PROPERTY_MAX_HIT_POINTS(M_HIT_POINTS),
        OBJECT_PROPERTY(
            M_PRIV, guides_lara, false,
            "Whether he waits for Lara and lectures her at the markers he "
            "walks to, rather than running them on his own."));
}

REGISTER_OBJECT(O_VON_CROY, M_Setup)
