#include <trx/core/benchmark.h>
#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/demo.h>
#include <trx/game/inject.h>
#include <trx/game/level/sections/read.h>
#include <trx/game/sound.h>

static void M_ReadPosition(XYZ_32 *const pos, VFILE *const file)
{
    pos->x = VFile_ReadS32(file);
    pos->y = VFile_ReadS32(file);
    pos->z = VFile_ReadS32(file);
}

static void M_ReadVertex(XYZ_16 *const vertex, VFILE *const file)
{
    vertex->x = VFile_ReadS16(file);
    vertex->y = VFile_ReadS16(file);
    vertex->z = VFile_ReadS16(file);
}

static void M_ReadObjectVector(OBJECT_VECTOR *const obj, VFILE *const file)
{
    M_ReadPosition(&obj->pos, file);
    obj->data = VFile_ReadS16(file);
    obj->flags = VFile_ReadS16(file);
}

void Level_Section_ReadCinematicFrames(
    LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int16_t num_frames = VFile_ReadS16(file);
    const int32_t inj_frames = Inject_GetDataCount(IDT_CINEMATIC_FRAMES);
    LOG_INFO("cinematic frames: %d", num_frames);
    Camera_InitialiseCineFrames(MAX(num_frames, inj_frames));
    for (int32_t i = 0; i < num_frames; i++) {
        CINE_FRAME *const frame = Camera_GetCineFrame(i);
        M_ReadVertex(&frame->target.shift, file);
        M_ReadVertex(&frame->camera.shift, file);
        frame->fov = VFile_ReadS16(file);
        frame->roll = VFile_ReadS16(file);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadCamerasAndSinks(
    LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_objects = VFile_ReadS32(file);
    LOG_DEBUG("fixed cameras/sinks: %d", num_objects);
    Camera_InitialiseFixedObjects(num_objects);
    for (int32_t i = 0; i < num_objects; i++) {
        M_ReadObjectVector(Camera_GetFixedObject(i), file);
    }

    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadFlybyCameras(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int16_t num_cameras = VFile_ReadS16(file);
    VFile_Skip(file, sizeof(int16_t)); // reserved padding
    Camera_InitialiseFlybys(num_cameras); // TODO: detect injections
    LOG_INFO("flyby cameras: %d", num_cameras);

    for (int32_t i = 0; i < num_cameras; i++) {
        FLYBY_CAMERA *const camera = Camera_GetFlybyCamera(i);
        M_ReadPosition(&camera->pos, file);
        M_ReadPosition(&camera->target, file);
        camera->sequence = VFile_ReadU8(file);
        camera->index = VFile_ReadU8(file);
        camera->fov = VFile_ReadS16(file);
        camera->roll = VFile_ReadS16(file);
        camera->timer = VFile_ReadS16(file);
        camera->speed = VFile_ReadS16(file);
        const int16_t flags = VFile_ReadS16(file);
        camera->room_num = VFile_ReadS16(file);
        VFile_Skip(file, sizeof(int16_t));

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

    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadDemoData(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const uint16_t size = VFile_ReadU16(file);
    LOG_INFO("demo buffer size: %d", size);
    Demo_LoadData(file, size);
    Benchmark_End(&benchmark, nullptr);
}

void Level_Section_ReadSoundSources(LEVEL_CONTEXT *const ctx, VFILE *const file)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int32_t num_sources = VFile_ReadS32(file);
    LOG_INFO("sound sources: %d", num_sources);
    Sound_InitialiseSources(num_sources);
    for (int32_t i = 0; i < num_sources; i++) {
        M_ReadObjectVector(Sound_GetSource(i), file);
    }

    Benchmark_End(&benchmark, nullptr);
}
