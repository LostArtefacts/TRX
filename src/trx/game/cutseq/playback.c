#include <trx/game/cutseq/playback.h>

#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/debug.h>
#include <trx/game/anims/walk.h>
#include <trx/game/camera.h>
#include <trx/game/clock.h>
#include <trx/game/const.h>
#include <trx/game/cutseq/decoder.h>
#include <trx/game/cutseq/pak.h>
#include <trx/game/fader.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/game_flow.h>
#include <trx/game/input.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/lara/pose.h>
#include <trx/game/lua/events.h>
#include <trx/game/matrix.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/overlay.h>
#include <trx/game/rooms.h>
#include <trx/game/viewport.h>
#include <trx/version.h>

#define M_NO_CUTSCENE (-1)
#define M_NO_FRAME (-1)
// The OG cutscene FOV; normal gameplay uses 14560.
#define M_DEFAULT_FOV 11488
#define M_FADE_DURATION 0.5f
// The OG fades out over the last 8 frames of the track.
#define M_END_FADE_FRAMES 8
#define M_END_FADE_DURATION (M_END_FADE_FRAMES / (float)LOGIC_FPS)
// The OG cinematic border (SetFadeClip) is 28 lines of a 480-line screen.
#define M_DEFAULT_LETTERBOX (28.0f / 480.0f)
// A title level shows none of it: the OG guards both the set and the clear
// with gfCurrentLevel, which is 0 there.
#define M_TITLE_LETTERBOX 0.0f

typedef enum {
    M_PHASE_INACTIVE,
    M_PHASE_FADE_OUT, // fading the scene out before the first frame
    M_PHASE_PLAYING,
    M_PHASE_FADE_END, // final frames, fading to black
} M_PHASE;

typedef struct {
    OBJECT_ID obj_id;
    int16_t src_node;
} M_MESH_OVERRIDE;

typedef struct {
    CUTSEQ_PACK_NODE *nodes;
    int32_t node_count;
    CUTSEQ_POSE pose_prev;
    CUTSEQ_POSE pose_curr;
    CUTSEQ_POSE pose_draw;
    bool is_hidden;
    M_MESH_OVERRIDE overrides[CUTSEQ_MAX_MESHES];
    ITEM dummy_item;
} M_ACTOR;

static const BOUNDS_16 m_LaraShadowBounds = {
    .min = { .x = -165, .y = -777, .z = -87 },
    .max = { .x = 150, .y = 1, .z = 78 },
};

static struct {
    M_PHASE phase;
    int32_t num;
    int32_t pending_num;
    int32_t frame;
    int32_t decoded_frames;
    CUTSEQ_INFO info;
    CUTSEQ_PACK_NODE camera_nodes[2];
    M_ACTOR actors[CUTSEQ_MAX_ACTORS];
    LARA_POSE lara_pose;
    CAMERA_TYPE old_cam_type;
    // Where the scene leaves Lara. Present once a script has named the place,
    // rather than the request having taken where it found her.
    struct {
        XYZ_32 pos;
        int16_t rot;
        bool is_present;
    } lara_return;
    struct {
        BOUNDS_16 value;
        bool is_present;
    } lara_shadow_bounds;
    uint64_t played_mask;
    int32_t fov;
    float letterbox;
    FADER fader;
} m_State = {
    .num = M_NO_CUTSCENE,
    .pending_num = M_NO_CUTSCENE,
    .fov = M_DEFAULT_FOV,
    .letterbox = M_DEFAULT_LETTERBOX,
};

// Root offsets are truncated to int16 to match the original engine, so a
// track running past the boundary wraps in a single frame. Interpolating the
// short way around turns that into a step rather than a sweep back across the
// whole range.
static int32_t M_LerpOffset(
    const int32_t from, const int32_t to, const double alpha)
{
    return from + (int32_t)((int16_t)(to - from) * alpha);
}

static void M_LerpPose(
    const CUTSEQ_POSE *const from, const CUTSEQ_POSE *const to,
    const double alpha, CUTSEQ_POSE *const out, const int32_t mesh_count)
{
    out->offset.x = M_LerpOffset(from->offset.x, to->offset.x, alpha);
    out->offset.y = M_LerpOffset(from->offset.y, to->offset.y, alpha);
    out->offset.z = M_LerpOffset(from->offset.z, to->offset.z, alpha);
    for (int32_t i = 0; i < mesh_count; i++) {
        out->rots[i] = Matrix_SlerpRot16(from->rots[i], to->rots[i], alpha);
    }
}

static void M_SetLaraPose(const CUTSEQ_POSE *const pose)
{
    m_State.lara_pose.offset = (XYZ_16) {
        .x = pose->offset.x,
        .y = pose->offset.y,
        .z = pose->offset.z,
    };
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        m_State.lara_pose.rots[i] = pose->rots[i];
    }
    Lara_Pose_SetOverride(&m_State.lara_pose);
}

// An actor names its object by the number its level gives it. The catalog
// answers for the ones TRX has an entry for, and the level's own slots for the
// rest: a TR4 wad parks cutscene-only geometry wherever it had a slot spare.
static const OBJECT *M_GetActorObject(const CUTSEQ_ACTOR_INFO *const actor_info)
{
    const OBJECT *const obj = Object_TryGet(actor_info->obj_id);
    if (obj != nullptr && obj->loaded) {
        return obj;
    }
    return Object_GetUncatalogedSlot(actor_info->game_obj_slot);
}

static float M_GetDefaultLetterbox(void)
{
    const GF_LEVEL *const level = GF_GetCurrentLevel();
    return level != nullptr && level->type == GFL_TITLE ? M_TITLE_LETTERBOX
                                                        : M_DEFAULT_LETTERBOX;
}

static void M_ReleaseNodes(void)
{
    for (int32_t i = 0; i < CUTSEQ_MAX_ACTORS; i++) {
        Memory_FreePointer(&m_State.actors[i].nodes);
        m_State.actors[i].node_count = 0;
    }
}

// Snaps Lara to a position in a neutral standing state, like the OG does when
// entering and leaving cutscenes.
static void M_TeleportLara(const XYZ_32 pos, const int16_t y_rot)
{
    ITEM *const item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();

    item->pos = pos;
    item->rot = (XYZ_16) { .y = y_rot };
    lara->head_rot = (XYZ_16) {};
    lara->torso_rot = (XYZ_16) {};

    const int16_t room_num = Room_GetIndexFromPos(pos);
    if (room_num != NO_ROOM && room_num != item->room_num) {
        Item_UpdateRoom(Item_GetIndex(item), room_num);
    }

    Item_SwitchToAnim(item, LA(LA_STAND_STILL), 0);
    item->current_anim_state = Item_GetAnim(item)->current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->required_anim_state = 0;
    item->speed = 0;
    item->fall_speed = 0;
    item->gravity = false;
    lara->gun_status = LGS_ARMLESS;
    g_Camera.fixed_camera = true;
    Interpolation_CommitLara();
}

static bool M_Begin(const int32_t num)
{
    // Read into a local, so a descriptor that turns out to be malformed does
    // not become the state a later teardown reads its audio track out of.
    CUTSEQ_INFO parsed;
    CUTSEQ_INFO *const info = &parsed;
    if (Lara_GetItem() == nullptr) {
        LOG_ERROR("Cannot play cutscene %d without Lara", num);
        return false;
    }
    if (!CutSeq_Pak_GetCutscene(num, info)) {
        LOG_ERROR("Failed to load cutscene %d", num);
        return false;
    }

    M_ReleaseNodes();
    for (int32_t i = 0; i < info->num_actors; i++) {
        M_ACTOR *const actor = &m_State.actors[i];
        const CUTSEQ_ACTOR_INFO *const actor_info = &info->actors[i];
        // Every node the actor declares is parsed, because the track data
        // starts after the whole header block; posing and drawing are what
        // CUTSEQ_MAX_MESHES bounds.
        const int32_t node_count = actor_info->node_count + 1;
        actor->node_count = node_count;
        actor->nodes = Memory_Alloc(node_count * sizeof(CUTSEQ_PACK_NODE));
        if (CutSeq_Decoder_InitNodes(
                info->data + actor_info->data_offset,
                info->data_size - actor_info->data_offset, actor->nodes,
                node_count)
            < 0) {
            LOG_ERROR("Malformed cutscene %d actor %d tracks", num, i);
            M_ReleaseNodes();
            return false;
        }
        CutSeq_Decoder_Reset(actor->nodes, node_count);
        CutSeq_Decoder_BuildPose(actor->nodes, node_count, &actor->pose_curr);
        actor->pose_prev = actor->pose_curr;
        actor->pose_draw = actor->pose_curr;

        actor->is_hidden = false;
        for (int32_t j = 0; j < CUTSEQ_MAX_MESHES; j++) {
            actor->overrides[j].obj_id = NO_OBJECT;
        }
        actor->dummy_item = (ITEM) {
            .object_id = actor_info->obj_id,
            .room_num = Lara_GetItem()->room_num,
            .pos = info->origin,
        };

        if (i > 0 && M_GetActorObject(actor_info) == nullptr) {
            LOG_WARNING(
                "Cutscene %d actor %d (TR4 slot %d) has no loaded object", num,
                i, actor_info->game_obj_slot);
        }
    }
    if (CutSeq_Decoder_InitNodes(
            info->data + info->camera_offset,
            info->data_size - info->camera_offset, m_State.camera_nodes, 2)
        < 0) {
        LOG_ERROR("Malformed cutscene %d camera tracks", num);
        M_ReleaseNodes();
        return false;
    }
    CutSeq_Decoder_Reset(m_State.camera_nodes, 2);

    LOG_DEBUG(
        "Playing cutscene %d: %d actors, %d frames, audio track %d", num,
        info->num_actors, info->num_frames, info->audio_track);

    m_State.info = parsed;
    m_State.num = num;
    m_State.frame = 0;
    m_State.decoded_frames = 0;
    // Taken here rather than at the request, so a camera trigger hit during
    // the fade out is what the scene hands back to. A flyby is the exception:
    // its sequence is dropped below, and handing CAM_FLYBY_MODE back would
    // leave Camera_Update on a branch with no sequence to follow.
    m_State.old_cam_type =
        g_Camera.type == CAM_FLYBY_MODE ? CAM_CHASE : g_Camera.type;
    CutSeq_SetPlayed(num, true);
    m_State.lara_shadow_bounds.is_present = false;

    M_TeleportLara(info->origin, 0);
    M_SetLaraPose(&m_State.actors[0].pose_curr);

    // A cutscene a flyby triggered begins while that flyby still holds the
    // camera. Stopping it rather than deactivating it leaves the camera where
    // the sequence left it, for the fade to black to cover.
    if (FlybyMode_IsActive()) {
        FlybyMode_Stop();
    }
    g_Camera.type = CAM_CINEMATIC;
    g_Camera.pos.pos = info->origin;
    g_Camera.pos.room_num = Lara_GetItem()->room_num;
    Viewport_AlterFOV(m_State.fov, FOV_MODE_CUTSCENE);
    Lara_Hair_Initialise();

    if (info->audio_track != -1) {
        Music_Play_Direct((MUSIC_ID)info->audio_track, MPM_ONCE);
    }

    Output_Overlay_SetLetterbox(0.0f);
    Output_Overlay_SetFade(0.0f);

    Fader_InitTo(&m_State.fader, 1.0f, 0.0f, M_FADE_DURATION);
    m_State.phase = M_PHASE_PLAYING;
    LUA_FireEventInt32(LUA_EVENT_CUTSCENE_START, num);
    return true;
}

static void M_Abort(void)
{
    m_State.phase = M_PHASE_INACTIVE;
    m_State.num = M_NO_CUTSCENE;
    m_State.pending_num = M_NO_CUTSCENE;
    m_State.lara_return.is_present = false;
    m_State.lara_shadow_bounds.is_present = false;
    Fader_InitFromCurrent(&m_State.fader, 0.0f, M_FADE_DURATION);
}

static void M_Finish(void)
{
    const int32_t num = m_State.num;
    const CUTSEQ_INFO *const info = &m_State.info;
    if (info->audio_track != -1) {
        Music_StopTrack_Direct((MUSIC_ID)info->audio_track);
    }

    M_ReleaseNodes();
    Lara_Pose_SetOverride(nullptr);
    M_TeleportLara(m_State.lara_return.pos, m_State.lara_return.rot);
    m_State.lara_return.is_present = false;
    m_State.lara_shadow_bounds.is_present = false;
    Lara_Hair_Initialise();
    Interpolation_CommitLara();

    g_Camera.type = m_State.old_cam_type;
    Viewport_AlterFOV(-1, FOV_MODE_GAME);

    m_State.phase = M_PHASE_INACTIVE;
    m_State.num = M_NO_CUTSCENE;
    Fader_InitTo(&m_State.fader, 1.0f, 0.0f, M_FADE_DURATION);

    // Fired last, so a handler that starts the next thing - the title hands
    // over to a flyby here - sees the cutscene fully torn down.
    LUA_FireEventInt32(LUA_EVENT_CUTSCENE_END, num);
}

// One frame of the scene: decodes every track, poses the actors and Lara, and
// keeps the audio track in step. The camera reads what this leaves behind.
static void M_Step(void)
{
    const CUTSEQ_INFO *const info = &m_State.info;
    if (m_State.frame > 0 && m_State.decoded_frames < info->num_frames) {
        m_State.decoded_frames++;
        CutSeq_Decoder_Advance(m_State.camera_nodes, 2, 0xFFFF);
        for (int32_t i = 0; i < info->num_actors; i++) {
            M_ACTOR *const actor = &m_State.actors[i];
            actor->pose_prev = actor->pose_curr;
            CutSeq_Decoder_Advance(actor->nodes, actor->node_count, 1023);
            CutSeq_Decoder_BuildPose(
                actor->nodes, actor->node_count, &actor->pose_curr);
        }
    } else {
        for (int32_t i = 0; i < info->num_actors; i++) {
            M_ACTOR *const actor = &m_State.actors[i];
            actor->pose_prev = actor->pose_curr;
        }
    }

    M_SetLaraPose(&m_State.actors[0].pose_curr);
    Lara_Hair_Control(true);

    if (info->audio_track != -1
        && Music_GetCurrentPlayingTrack() == (MUSIC_ID)info->audio_track) {
        IGNORE(Music_SetSpeed(Clock_GetSpeedMultiplier()));
        IGNORE(Music_SyncTimestamp(m_State.frame / (double)LOGIC_FPS));
    }

    m_State.frame++;
    if (m_State.frame > info->num_frames - M_END_FADE_FRAMES
        && m_State.phase == M_PHASE_PLAYING) {
        m_State.phase = M_PHASE_FADE_END;
        Fader_InitFromCurrent(&m_State.fader, 1.0f, M_END_FADE_DURATION);
    }
    CLAMPG(m_State.frame, info->num_frames);
}

void CutSeq_Load(void)
{
    m_State.letterbox = M_GetDefaultLetterbox();

    if (g_TRVersion != 4 || !CutSeq_Pak_Load()) {
        return;
    }
    const int32_t count = CutSeq_Pak_GetCutsceneCount();
    if (count > CUTSEQ_MAX_CUTSCENES) {
        LOG_WARNING(
            "cutseq.pak holds %d cutscenes; only the first %d are playable",
            count, CUTSEQ_MAX_CUTSCENES);
    }
}

bool CutSeq_IsAvailable(void)
{
    return g_TRVersion == 4 && CutSeq_Pak_IsLoaded();
}

int32_t CutSeq_GetCount(void)
{
    if (!CutSeq_IsAvailable()) {
        return 0;
    }
    return MIN(CutSeq_Pak_GetCutsceneCount(), CUTSEQ_MAX_CUTSCENES);
}

bool CutSeq_IsActive(void)
{
    return m_State.phase != M_PHASE_INACTIVE;
}

bool CutSeq_IsPlaying(void)
{
    return m_State.phase == M_PHASE_PLAYING
        || m_State.phase == M_PHASE_FADE_END;
}

bool CutSeq_IsFading(void)
{
    return Fader_IsActive(&m_State.fader);
}

int32_t CutSeq_GetCurrent(void)
{
    return CutSeq_IsPlaying() ? m_State.num : M_NO_CUTSCENE;
}

int32_t CutSeq_GetFrame(void)
{
    return CutSeq_IsPlaying() ? m_State.frame : M_NO_FRAME;
}

void CutSeq_Request(const int32_t num, const bool fade_out)
{
    // Lara is a cutscene's first actor, so a level that never placed her - a
    // title that only shows scenery - has nothing to play.
    if (CutSeq_IsActive() || num < 0 || num >= CutSeq_GetCount()
        || Lara_GetItem() == nullptr) {
        return;
    }

    // A script may name the place before asking for the scene as readily as
    // during it.
    if (!m_State.lara_return.is_present) {
        m_State.lara_return.pos = Lara_GetItem()->pos;
        m_State.lara_return.rot = Lara_GetItem()->rot.y;
    }
    m_State.pending_num = num;
    m_State.phase = M_PHASE_FADE_OUT;
    if (fade_out) {
        Fader_InitFromCurrent(&m_State.fader, 1.0f, M_FADE_DURATION);
    } else {
        // Black at once rather than over time, so the next tick begins the
        // scene and M_Begin's own fade brings it in from there.
        Fader_InitTo(&m_State.fader, 1.0f, 1.0f, 0.0f);
    }
}

void CutSeq_Skip(void)
{
    if (m_State.phase != M_PHASE_PLAYING) {
        return;
    }
    m_State.phase = M_PHASE_FADE_END;
    Fader_InitFromCurrent(&m_State.fader, 1.0f, M_END_FADE_DURATION);
}

void CutSeq_HandleTrigger(const int32_t num)
{
    // A pad answers every frame Lara stands on it, so the once-only rule comes
    // first - it decides whether there is a trigger to answer at all, and a
    // script that wants a scene again clears its played mark.
    if (CutSeq_IsActive() || CutSeq_IsPlayed(num)) {
        return;
    }

    // Marked before anything acts on it, as the original engine marks it, so
    // that a number nothing can answer - one the pak holds no scene for, an
    // FMV a script declines to play - still stops asking.
    CutSeq_SetPlayed(num, true);

    // A script gets the trigger first, and a handler that returns true has
    // answered it: with a cutscene of its own, a flyby, or nothing at all.
    if (LUA_FireEventInt32(LUA_EVENT_CUTSCENE_TRIGGER, num)) {
        return;
    }
    CutSeq_Request(num, true);
}

uint64_t CutSeq_GetPlayedMask(void)
{
    return m_State.played_mask;
}

void CutSeq_SetPlayedMask(const uint64_t mask)
{
    m_State.played_mask = mask;
}

bool CutSeq_IsPlayed(const int32_t num)
{
    return num >= 0 && num < CUTSEQ_MAX_TRIGGERS
        && (m_State.played_mask & (1ull << num)) != 0;
}

void CutSeq_SetPlayed(const int32_t num, const bool played)
{
    if (num < 0 || num >= CUTSEQ_MAX_TRIGGERS) {
        return;
    }
    if (played) {
        m_State.played_mask |= 1ull << num;
    } else {
        m_State.played_mask &= ~(1ull << num);
    }
}

int32_t CutSeq_GetActorCount(void)
{
    return CutSeq_IsPlaying() ? m_State.info.num_actors : 0;
}

void CutSeq_SetActorVisible(const int32_t actor, const bool visible)
{
    if (actor < 0 || actor >= CUTSEQ_MAX_ACTORS) {
        return;
    }
    m_State.actors[actor].is_hidden = !visible;
}

void CutSeq_SetActorNodeMesh(
    const int32_t actor, const int32_t node, const OBJECT_ID obj_id,
    const int32_t src_node)
{
    if (actor < 0 || actor >= CUTSEQ_MAX_ACTORS || node < 0
        || node >= CUTSEQ_MAX_MESHES) {
        return;
    }
    m_State.actors[actor].overrides[node] = (M_MESH_OVERRIDE) {
        .obj_id = obj_id,
        .src_node = src_node,
    };
}

const BOUNDS_16 *CutSeq_GetLaraShadowBounds(void)
{
    if (!CutSeq_IsPlaying()) {
        return nullptr;
    }
    return m_State.lara_shadow_bounds.is_present
        ? &m_State.lara_shadow_bounds.value
        : &m_LaraShadowBounds;
}

void CutSeq_SetLaraShadowBounds(const BOUNDS_16 bounds)
{
    m_State.lara_shadow_bounds.value = bounds;
    m_State.lara_shadow_bounds.is_present = true;
}

void CutSeq_SetLaraReturn(const XYZ_32 pos, const int16_t rot)
{
    m_State.lara_return.pos = pos;
    m_State.lara_return.rot = rot;
    m_State.lara_return.is_present = true;
}

int32_t CutSeq_GetFOV(void)
{
    return m_State.fov;
}

void CutSeq_SetFOV(const int32_t fov)
{
    m_State.fov = fov;
}

float CutSeq_GetLetterbox(void)
{
    return m_State.letterbox;
}

void CutSeq_SetLetterbox(const float ratio)
{
    m_State.letterbox = ratio;
}

void CutSeq_Reset(void)
{
    const int32_t num = m_State.num;
    if (num != M_NO_CUTSCENE && m_State.info.audio_track != -1) {
        Music_StopTrack_Direct((MUSIC_ID)m_State.info.audio_track);
    }

    M_ReleaseNodes();
    Lara_Pose_SetOverride(nullptr);
    if (num != M_NO_CUTSCENE) {
        g_Camera.type = m_State.old_cam_type;
        Viewport_AlterFOV(-1, FOV_MODE_GAME);
    }
    m_State.phase = M_PHASE_INACTIVE;
    m_State.num = M_NO_CUTSCENE;
    m_State.pending_num = M_NO_CUTSCENE;
    m_State.frame = 0;
    m_State.decoded_frames = 0;
    m_State.lara_return.is_present = false;
    m_State.info = (CUTSEQ_INFO) {};
    m_State.fader = (FADER) {};
    // How a scene is framed is a script's setting, and a script lasts as long
    // as its level. Which cutscenes have run is the playthrough's, and stays.
    m_State.fov = M_DEFAULT_FOV;

    // A cutscene dropped part-way through still ends, so a script that pairs
    // the two events - the title hides Lara between them - is not left holding
    // what it set up.
    if (num != M_NO_CUTSCENE) {
        LUA_FireEventInt32(LUA_EVENT_CUTSCENE_END, num);
    }

    // A handler answering that event by asking for another scene - the one a
    // chain names - is asking it of the level being taken apart, so nothing it
    // asked for is carried into the next one.
    m_State.phase = M_PHASE_INACTIVE;
    m_State.pending_num = M_NO_CUTSCENE;
    m_State.fader = (FADER) {};
}

void CutSeq_Control(void)
{
    // As a cutscene level answers them, and for the same reason: a scene the
    // player has seen before is a scene to get past.
    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        CutSeq_Skip();
        Input_HoldOffSkip();
    }

    switch (m_State.phase) {
    case M_PHASE_FADE_OUT:
        if (!Fader_IsActive(&m_State.fader)) {
            const int32_t num = m_State.pending_num;
            m_State.pending_num = M_NO_CUTSCENE;
            if (!M_Begin(num)) {
                M_Abort();
            }
        }
        // A scene that just began is at frame 0, and its first step is the
        // next tick, so nothing is stepped here either way.
        return;

    case M_PHASE_FADE_END:
        if (!Fader_IsActive(&m_State.fader)) {
            M_Finish();
            return;
        }
        break;

    case M_PHASE_PLAYING:
        break;

    default:
        return;
    }

    M_Step();
}

void CutSeq_PostControl(void)
{
    // Fixed-camera triggers hit by items during the cutscene would hijack
    // the camera type and stall the playback; the OG forces the cinematic
    // camera every tick for the same reason.
    if (CutSeq_IsPlaying()) {
        g_Camera.type = CAM_CINEMATIC;
    }
}

void CutSeq_UpdateCamera(void)
{
    if (!CutSeq_IsPlaying()) {
        return;
    }

    const CUTSEQ_INFO *const info = &m_State.info;
    GAME_VECTOR old_pos = g_Camera.pos;
    g_Camera.target.pos = (XYZ_32) {
        .x = info->origin.x + 2 * m_State.camera_nodes[0].x_run,
        .y = info->origin.y + 2 * m_State.camera_nodes[0].y_run,
        .z = info->origin.z + 2 * m_State.camera_nodes[0].z_run,
    };
    g_Camera.pos.pos = (XYZ_32) {
        .x = info->origin.x + 2 * m_State.camera_nodes[1].x_run,
        .y = info->origin.y + 2 * m_State.camera_nodes[1].y_run,
        .z = info->origin.z + 2 * m_State.camera_nodes[1].z_run,
    };

    const int16_t room_num =
        Room_FindByTraversal(old_pos.pos, g_Camera.pos.pos, old_pos.room_num);
    if (room_num != NO_ROOM) {
        g_Camera.pos.room_num = room_num;
    }
    g_Camera.target.room_num = g_Camera.pos.room_num;
    Room_GetSector(g_Camera.target.pos, &g_Camera.target.room_num);
    g_Camera.roll = 0;
    g_Camera.shift = 0;
    Viewport_AlterFOV(m_State.fov, FOV_MODE_CUTSCENE);
    Camera_UpdateMicPosition();
}

void CutSeq_PreDraw(void)
{
    if (!CutSeq_IsPlaying()) {
        return;
    }

    double alpha = Interpolation_GetWorldRate();
    if (alpha < 0.0 || alpha > 1.0) {
        alpha = 1.0;
    }

    for (int32_t i = 0; i < m_State.info.num_actors; i++) {
        M_ACTOR *const actor = &m_State.actors[i];
        M_LerpPose(
            &actor->pose_prev, &actor->pose_curr, alpha, &actor->pose_draw,
            MIN(actor->node_count - 1, CUTSEQ_MAX_MESHES));
    }
    M_SetLaraPose(&m_State.actors[0].pose_draw);
}

void CutSeq_DrawActors(void)
{
    if (!CutSeq_IsPlaying()) {
        return;
    }

    const CUTSEQ_INFO *const info = &m_State.info;
    for (int32_t i = 1; i < info->num_actors; i++) {
        M_ACTOR *const actor = &m_State.actors[i];
        const OBJECT *const obj = M_GetActorObject(&info->actors[i]);
        if (obj == nullptr || actor->is_hidden) {
            continue;
        }
        const CUTSEQ_POSE *const pose = &actor->pose_draw;

        const XYZ_32 light_pos = {
            .x = info->origin.x + pose->offset.x,
            .y = info->origin.y + pose->offset.y,
            .z = info->origin.z + pose->offset.z,
        };
        const int16_t room_num = Room_GetIndexFromPos(light_pos);
        ITEM *const dummy = &actor->dummy_item;
        if (room_num != NO_ROOM) {
            dummy->room_num = room_num;
        }
        dummy->pos = light_pos;
        Output_SetCurrentRoom(Room_Get(dummy->room_num));
        Output_CalculateObjectLightingAt(
            dummy,
            (GAME_VECTOR) { .pos = light_pos, .room_num = dummy->room_num });

        Matrix_Push();
        Matrix_TranslateAbs32(info->origin);
        Matrix_TranslateRel32(pose->offset);

        const int32_t mesh_count =
            MIN(MIN(obj->mesh_count, actor->node_count - 1), CUTSEQ_MAX_MESHES);

        ANIM_WALK walk;
        Anim_Walk_BeginToJoint(
            &walk,
            &(ANIM_WALK_DESC) {
                .obj = obj,
                .pose = Anim_Pose_FromRots(pose->rots, (XYZ_16) {}),
            },
            mesh_count - 1);
        while (Anim_Walk_Next(&walk)) {
            const M_MESH_OVERRIDE *const override =
                &actor->overrides[walk.joint];
            const OBJECT *const src_obj = override->obj_id == NO_OBJECT
                ? nullptr
                : Object_TryGet(override->obj_id);
            if (src_obj != nullptr && src_obj->loaded) {
                Object_DrawMesh(
                    src_obj->mesh_idx + override->src_node, CLIP_FULLY_VISIBLE,
                    false);
            } else {
                Object_DrawMesh(
                    obj->mesh_idx + walk.joint, CLIP_FULLY_VISIBLE, false);
            }
        }
        Anim_Walk_End(&walk);

        Matrix_Pop();
    }
}

void CutSeq_DrawOverlay(void)
{
    if (CutSeq_IsPlaying()) {
        const int32_t height = Viewport_GetHeight(VIEWPORT_UI);
        const int32_t width = Viewport_GetWidth(VIEWPORT_UI);
        const int32_t bar_height = (int32_t)(height * m_State.letterbox);
        const RGBA_8888 black = { 0, 0, 0, 255 };
        Output_DrawScreenFlatQuad(0, 0, 0, width, bar_height, black);
        Output_DrawScreenFlatQuad(
            0, height - bar_height, 0, width, bar_height, black);
    }

    const float fade = Fader_GetCurrentValue(&m_State.fader);
    if (fade > 0.0f) {
        Output_Overlay_DrawBlackRectangle(fade, false);
    }
}
