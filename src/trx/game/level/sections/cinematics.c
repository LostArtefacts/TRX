#include <trx/core/benchmark.h>
#include <trx/core/file.h>
#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/demo.h>
#include <trx/game/inject.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/sound.h>

static void M_ReadPosition(XYZ_32 *const pos, TRX_FILE *const file)
{
    pos->x = File_ReadS32(file);
    pos->y = File_ReadS32(file);
    pos->z = File_ReadS32(file);
}

static void M_ReadVertex(XYZ_16 *const vertex, TRX_FILE *const file)
{
    vertex->x = File_ReadS16(file);
    vertex->y = File_ReadS16(file);
    vertex->z = File_ReadS16(file);
}

static void M_ReadObjectVector(OBJECT_VECTOR *const obj, TRX_FILE *const file)
{
    M_ReadPosition(&obj->pos, file);
    obj->data = File_ReadS16(file);
    obj->flags = File_ReadS16(file);
}

RESULT Level_Section_ReadCinematicFrames(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int16_t num_frames = File_ReadCountS16(file);
    const int32_t inj_frames = Inject_GetDataCount(IDT_CINEMATIC_FRAMES);
    LOG_INFO("cinematic frames: %d", num_frames);
    Camera_InitialiseCineFrames(MAX(num_frames, inj_frames));
    for (int32_t i = 0; i < num_frames; i++) {
        CINE_FRAME *const frame = Camera_GetCineFrame(i);
        M_ReadVertex(&frame->target.shift, file);
        M_ReadVertex(&frame->camera.shift, file);
        frame->fov = File_ReadS16(file);
        frame->roll = File_ReadS16(file);
    }

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

RESULT Level_Section_ReadCamerasAndSinks(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_objects = File_ReadCountS32(file);
    LOG_DEBUG("fixed cameras/sinks: %d", num_objects);
    Camera_InitialiseFixedObjects(num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        M_ReadObjectVector(Camera_GetFixedObject(i), file);
    }

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_PrepareTR123FlybyCameras(void)
{
    const int32_t inject_count = Inject_GetDataCount(IDT_FLYBY_CAMERAS);
    Camera_InitialiseFlybys(inject_count);
}

RESULT Level_Section_ReadFlybyCameras(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int16_t num_cameras = File_ReadCountS16(file);
    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    info->cameras.flyby_count = num_cameras;
    File_Skip(file, sizeof(int16_t)); // reserved padding
    Camera_InitialiseFlybys(
        num_cameras + Inject_GetDataCount(IDT_FLYBY_CAMERAS));
    LOG_INFO("flyby cameras: %d", num_cameras);
    Level_Section_AppendFlybyCameras(0, num_cameras, file);

    Benchmark_End(&benchmark, nullptr);
    return OK;
}

void Level_Section_AppendFlybyCameras(
    const int32_t base_idx, const int32_t num_cameras, TRX_FILE *const file)
{
    for (int32_t i = 0; i < num_cameras; i++) {
        FLYBY_CAMERA *const camera = Camera_GetFlybyCamera(base_idx + i);
        M_ReadPosition(&camera->pos, file);
        M_ReadPosition(&camera->target, file);
        camera->sequence = File_ReadU8(file);
        camera->index = File_ReadU8(file);
        camera->fov = File_ReadS16(file);
        camera->roll = File_ReadS16(file);
        camera->timer = File_ReadS16(file);
        camera->speed = File_ReadS16(file);
        const int16_t flags = File_ReadS16(file);
        camera->room_num = File_ReadS16(file);
        File_Skip(file, sizeof(int16_t));

        // clang-format off
        camera->flags.snap_from_game   = (flags & 0x0001) != 0;
        camera->flags.target_item      = (flags & 0x0002) != 0;
        camera->flags.loop             = (flags & 0x0004) != 0;
        camera->flags.track_path       = (flags & 0x0008) != 0;
        camera->flags.focus_lara       = (flags & 0x0010) != 0;
        camera->flags.target_lara      = (flags & 0x0020) != 0;
        camera->flags.snap_to_game     = (flags & 0x0040) != 0;
        camera->flags.jump_to_camera   = (flags & 0x0080) != 0;
        camera->flags.hold             = (flags & 0x0100) != 0;
        camera->flags.no_break         = (flags & 0x0200) != 0;
        camera->flags.lara_control_off = (flags & 0x0400) != 0;
        camera->flags.lara_control_on  = (flags & 0x0800) != 0;
        camera->flags.fade_in_screen   = (flags & 0x1000) != 0;
        camera->flags.fade_out_screen  = (flags & 0x2000) != 0;
        camera->flags.test_triggers    = (flags & 0x4000) != 0;
        camera->flags.one_shot         = (flags & 0x8000) != 0;
        // clang-format on
    }
}

RESULT Level_Section_ReadDemoData(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const uint16_t size = File_ReadU16(file);
    LOG_INFO("demo buffer size: %d", size);
    Demo_LoadData(file, size);
    Benchmark_End(&benchmark, nullptr);
    return OK;
}

RESULT Level_Section_ReadSoundSources(
    LEVEL_CONTEXT *const ctx, TRX_FILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_sources = File_ReadCountS32(file);
    LOG_INFO("sound sources: %d", num_sources);
    Sound_InitialiseSources(num_sources);
    for (int32_t i = 0; i < num_sources; i++) {
        M_ReadObjectVector(Sound_GetSource(i), file);
    }

    Benchmark_End(&benchmark, nullptr);
    return OK;
}
