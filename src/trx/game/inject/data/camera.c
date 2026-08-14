#include <trx/core/file.h>
#include <trx/debug.h>
#include <trx/game/camera/cinematic.h>
#include <trx/game/inject.h>

static void M_ReadVertex(XYZ_16 *const vertex, TRX_FILE *const file)
{
    vertex->x = File_ReadS16(file);
    vertex->y = File_ReadS16(file);
    vertex->z = File_ReadS16(file);
}

static void M_HandleCineFrames(
    const INJECTION *const injection, const int32_t data_count)
{
    for (int32_t i = 0; i < data_count; i++) {
        CINE_FRAME *const frame = Camera_GetCineFrame(i);
        M_ReadVertex(&frame->target.shift, injection->fp);
        M_ReadVertex(&frame->camera.shift, injection->fp);
        frame->fov = File_ReadS16(injection->fp);
        frame->roll = File_ReadS16(injection->fp);
    }
}

static void M_HandleFlybyCameras(
    const INJECTION *const injection, const int32_t data_count)
{
    LEVEL_CONTEXT_INFO *const level_info = Level_Context_GetInfo();
    Level_Section_AppendFlybyCameras(
        level_info->cameras.flyby_count, data_count, injection->fp);
    level_info->cameras.flyby_count += data_count;
}

static void M_HandleCameraData(
    const INJECTION_CONTEXT *const ctx, const INJECTION_CHUNK chunk)
{
    for (int32_t i = 0; i < chunk.num_blocks; i++) {
        const INJECTION_DATA_TYPE data_type = File_ReadS32(chunk.injection->fp);
        const int32_t data_count = File_ReadS32(chunk.injection->fp);
        const int32_t data_size = File_ReadS32(chunk.injection->fp);

        if (ctx->mode == INJECTION_MODE_STATS) {
            File_Skip(chunk.injection->fp, data_size);
            continue;
        }

        switch (data_type) {
        case IDT_CINEMATIC_FRAMES:
            M_HandleCineFrames(chunk.injection, data_count);
            break;
        case IDT_FLYBY_CAMERAS:
            M_HandleFlybyCameras(chunk.injection, data_count);
            break;
        default:
            LOG_WARNING("Unknown data type: %d", data_type);
            File_Skip(chunk.injection->fp, data_size);
            break;
        }
    }
}

REGISTER_INJECTOR(ICT_CAMERA_DATA, M_HandleCameraData)
