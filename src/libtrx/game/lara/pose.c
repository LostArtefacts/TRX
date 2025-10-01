#include "game/lara/pose.h"

#include "debug.h"
#include "filesystem.h"
#include "game/game_flow.h"
#include "game/interpolation.h"
#include "game/items.h"
#include "game/lara/hair.h"
#include "game/objects.h"
#include "json_file.h"
#include "memory.h"
#include "vector.h"

#define M_NO_POSE (-1)

static const char *const m_Path = "cfg/poses.json5";
static VECTOR *m_Poses = nullptr;
static int32_t m_ActivePose = M_NO_POSE;

static bool M_ReadXYZ16(JSON_VALUE *const value, XYZ_16 *const target)
{
    JSON_OBJECT *const obj = JSON_ValueAsObject(value);
    if (obj != nullptr) {
        target->x = JSON_ObjectGetInt(obj, "x", 0);
        target->y = JSON_ObjectGetInt(obj, "y", 0);
        target->z = JSON_ObjectGetInt(obj, "z", 0);
        return true;
    }
    JSON_ARRAY *const arr = JSON_ValueAsArray(value);
    if (arr != nullptr && arr->length == 3) {
        target->x = JSON_ArrayGetInt(arr, 0, 0);
        target->y = JSON_ArrayGetInt(arr, 1, 0);
        target->z = JSON_ArrayGetInt(arr, 2, 0);
        return true;
    }
    return false;
}

static void M_LoadPoses(void)
{
    m_Poses = Vector_Create(sizeof(LARA_POSE));
    ASSERT(m_Poses != nullptr);

    JSON_VALUE *const doc = JSONFile_Read(m_Path);
    JSON_ARRAY *const poses = JSON_ValueAsArray(doc);
    if (poses == nullptr) {
        LOG_WARNING("Error while reading poses: root object must be an array");
        goto cleanup;
    }

    for (size_t i = 0; i < poses->length; ++i) {
        JSON_OBJECT *const pose_obj = JSON_ArrayGetObject(poses, i);
        LARA_POSE pose;
        if (!M_ReadXYZ16(
                JSON_ObjectGetValue(pose_obj, "offset"), &pose.offset)) {
            LOG_WARNING(
                "Error while reading pose %d: invalid or missing pose offset",
                i);
        }

        JSON_ARRAY *const rots_arr = JSON_ObjectGetArray(pose_obj, "rots");
        if (rots_arr == nullptr) {
            LOG_WARNING("Error while reading pose %d: missing rotations", i);
            continue;
        } else if (rots_arr->length != LM_NUMBER_OF) {
            LOG_WARNING(
                "Error while reading pose %d: expected exactly %d rotations, "
                "got %d",
                i, LM_NUMBER_OF, rots_arr->length);
            continue;
        } else {
            for (int32_t j = 0; j < LM_NUMBER_OF; j++) {
                if (!M_ReadXYZ16(
                        JSON_ArrayGetValue(rots_arr, j), &pose.rots[j])) {
                    LOG_WARNING(
                        "Error while reading pose %d: invalid or missing "
                        "rotation %d",
                        i, j);
                }
            }
        }

        Vector_Add(m_Poses, &pose);
    }

cleanup:
    JSON_ValueFree(doc);
}

void Lara_Pose_Init(void)
{
    if (m_Poses == nullptr) {
        M_LoadPoses();
    }
}

void Lara_Pose_Shutdown(void)
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
    if (m_ActivePose == M_NO_POSE) {
        return nullptr;
    }
    return Vector_Get(m_Poses, m_ActivePose);
}
