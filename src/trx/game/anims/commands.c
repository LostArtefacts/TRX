#include <trx/core/benchmark.h>
#include <trx/core/log.h>
#include <trx/game/anims.h>
#include <trx/game/game_buf.h>
#include <trx/version.h>

static int32_t M_GetCommandPayloadSize(const int16_t command_type)
{
    switch (command_type) {
    case AC_MOVE_ORIGIN:
        return 3;

    case AC_JUMP_VELOCITY:
        return 2;

    case AC_SOUND_FX:
    case AC_EFFECT:
        return 2;

    default:
        return 0;
    }
}

static bool M_ParseCommand(
    ANIM_COMMAND *const command, const int16_t **data, const int16_t *const end)
{
    const int16_t *data_ptr = *data;
    if (data_ptr >= end) {
        return false;
    }
    command->type = *data_ptr++;
    const int32_t payload_size = M_GetCommandPayloadSize(command->type);
    if (data_ptr + payload_size > end) {
        return false;
    }

    switch (command->type) {
    case AC_MOVE_ORIGIN: {
        XYZ_16 *const pos = GameBuf_Alloc(sizeof(XYZ_16), GBUF_ANIM_COMMANDS);
        pos->x = *data_ptr++;
        pos->y = *data_ptr++;
        pos->z = *data_ptr++;
        command->data = (void *)pos;
        break;
    }

    case AC_JUMP_VELOCITY: {
        ANIM_COMMAND_VELOCITY_DATA *const cmd_data = GameBuf_Alloc(
            sizeof(ANIM_COMMAND_VELOCITY_DATA), GBUF_ANIM_COMMANDS);
        cmd_data->fall_speed = *data_ptr++;
        cmd_data->speed = *data_ptr++;
        command->data = (void *)cmd_data;
        break;
    }

    case AC_SOUND_FX:
    case AC_EFFECT: {
        ANIM_COMMAND_EFFECT_DATA *const cmd_data =
            GameBuf_Alloc(sizeof(ANIM_COMMAND_EFFECT_DATA), GBUF_ANIM_COMMANDS);
        cmd_data->frame_num = *data_ptr++;
        const int16_t effect_data = *data_ptr++;
        cmd_data->effect_num = effect_data & 0x3FFF;
        cmd_data->fx_type = 0;
        if (command->type == AC_EFFECT && g_TRVersion == 3) {
            cmd_data->fx_type = effect_data & 0xC000;
            cmd_data->environment = ACE_ALL;
        } else {
            cmd_data->environment = (effect_data & 0xC000) >> 14;
        }
        command->data = (void *)cmd_data;
        break;
    }

    default:
        command->data = nullptr;
        break;
    }

    *data = data_ptr;
    return true;
}

void Anim_LoadCommands(const int16_t *data, const int32_t data_length)
{
    BENCHMARK benchmark = Benchmark_Start();
    const int16_t *const data_end = data + data_length;

    const int32_t anim_count = Anim_GetTotalCount();
    for (int32_t i = 0; i < anim_count; i++) {
        ANIM *const anim = Anim_GetAnim(i);
        if (anim->num_commands <= 0) {
            continue;
        }
        if (anim->command_idx < 0 || anim->command_idx >= data_length) {
            LOG_WARNING(
                "Anim %d has invalid command start %d (raw count: %d)", i,
                anim->command_idx, data_length);
            anim->num_commands = 0;
            continue;
        }

        anim->commands = GameBuf_Alloc(
            sizeof(ANIM_COMMAND) * anim->num_commands, GBUF_ANIM_COMMANDS);
        const int16_t *data_ptr = &data[anim->command_idx];
        int32_t parsed_count = 0;
        for (; parsed_count < anim->num_commands; parsed_count++) {
            ANIM_COMMAND *const command = &anim->commands[parsed_count];
            if (!M_ParseCommand(command, &data_ptr, data_end)) {
                LOG_WARNING(
                    "Anim %d command stream overruns raw data at cmd %d "
                    "(start: %d count: %d raw count: %d)",
                    i, parsed_count, anim->command_idx, anim->num_commands,
                    data_length);
                break;
            }
        }
        anim->num_commands = parsed_count;
    }

    Benchmark_End(&benchmark, nullptr);
}
