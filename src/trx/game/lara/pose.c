#include <trx/game/lara/pose.h>

#include <trx/core/filesystem.h>
#include <trx/core/json/util/file.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/game_flow.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara/hair.h>
#include <trx/game/objects.h>
#include <trx/game/shell.h>

#define M_NO_POSE (-1)

static VECTOR *m_Poses = nullptr;
static int32_t m_ActivePose = M_NO_POSE;
static LARA_POSE m_Override = {};
static bool m_OverrideActive = false;

static void M_WarnWithJSONError(const JSON_READ_IO *const io)
{
    char warning_message[1024];
    JSON_ReadIO_FormatError(
        io, false, warning_message, sizeof(warning_message));
    LOG_WARNING("%s", warning_message);
}

static RESULT M_LoadPose(JSON_READ_IO *const io, LARA_POSE *const pose)
{
    MUST(JSON_READ(io, "offset", &pose->offset));
    MUST(JSON_PUSH(io, "rots"));
    const int32_t rot_count = JSON_ARRAY_LEN(io);
    if (rot_count < 0) {
        MUST(JSON_POP(io));
        return JSON_ReadIO_Fail(io, "'rots' must be a list");
    }
    if (rot_count != LM_NUMBER_OF) {
        MUST(JSON_POP(io));
        return JSON_ReadIO_Fail(
            io, "'rots' needs exactly %d rotations, not %d", LM_NUMBER_OF,
            rot_count);
    }

    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        MUST(JSON_READ_A(io, i, &pose->rots[i]));
    }

    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_LoadPosesArray(JSON_READ_IO *const io, VECTOR *const poses)
{
    const int32_t pose_count = JSON_ARRAY_LEN(io);
    if (pose_count < 0) {
        return JSON_ReadIO_Fail(io, "the poses must be a list");
    }

    for (int32_t i = 0; i < pose_count; i++) {
        MUST(JSON_PUSH_INDEX(io, i));

        LARA_POSE pose = {};
        if (SHOULD(M_LoadPose(io, &pose))) {
            Vector_Add(poses, &pose);
        }
        MUST(JSON_POP(io));
    }

    return OK;
}

static RESULT M_LoadPoses(void)
{
    if (m_Poses != nullptr) {
        return OK;
    }
    m_Poses = Vector_Create(sizeof(LARA_POSE));
    ASSERT(m_Poses != nullptr);

    const char *const poses_path =
        GamePath_TryResolve(GAME_DYNAMIC_PATH_COMMON_CONFIG, "poses.json5");
    if (poses_path == nullptr) {
        return OK;
    }
    JSON_VALUE *doc = nullptr;
    SHOULD(JSONFile_Read(poses_path, &doc), "Lara keeps her default poses");
    if (doc == nullptr) {
        return OK;
    }

    JSON_READ_IO *const io = JSON_ReadIO_Create(doc, 0, poses_path);
    SHOULD(M_LoadPosesArray(io, m_Poses), "Lara keeps her default poses");
    JSON_ReadIO_Destroy(io);
    JSON_ValueFree(doc);
    return OK;
}

static void M_Shutdown(void)
{
    if (m_Poses != nullptr) {
        Vector_Free(m_Poses);
        m_Poses = nullptr;
    }
}

bool Lara_Pose_IsAvailable(void)
{
    return m_Poses->count > 0 && Object_Get(O_LARA)->loaded
        && GF_GetCurrentLevel()->type != GFL_CUTSCENE;
}

void Lara_Pose_Clear(void)
{
    if (m_ActivePose != M_NO_POSE) {
        LOG_DEBUG("Clearing Lara's pose");
    }
    m_ActivePose = M_NO_POSE;
}

void Lara_Pose_Cycle(const int32_t dir)
{
    if (!Lara_Pose_IsAvailable()) {
        return;
    }
    if (m_ActivePose == M_NO_POSE) {
        m_ActivePose = (dir > 0) ? 0 : m_Poses->count - 1;
    } else {
        m_ActivePose += dir;
        m_ActivePose += m_Poses->count;
        m_ActivePose %= m_Poses->count;
    }

    LOG_DEBUG("Active Lara pose: %d", m_ActivePose);
    Lara_Hair_Control(true);
    Interpolation_CommitBraid();
}

const LARA_POSE *Lara_Pose_Get(void)
{
    if (m_OverrideActive) {
        return &m_Override;
    }
    if (m_ActivePose == M_NO_POSE) {
        return nullptr;
    }
    return Vector_Get(m_Poses, m_ActivePose);
}

void Lara_Pose_SetOverride(const LARA_POSE *const pose)
{
    m_OverrideActive = pose != nullptr;
    if (pose != nullptr) {
        m_Override = *pose;
    }
}

REGISTER_SUBSYSTEM(.load = M_LoadPoses, .shutdown = M_Shutdown)
