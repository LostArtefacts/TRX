#include <trx/game/camera/environment.h>

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/game.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/output/vars.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/version.h>

static void M_AdjustMusicVolume(const bool is_underwater)
{
    if (!Game_IsPlaying()) {
        return;
    }
    const bool is_ambient =
        Music_GetCurrentPlayingTrack() == Music_GetCurrentLoopedTrack();
    const bool is_cutscene = GF_GetCurrentLevel()->type == GFL_CUTSCENE;
    const double base_volume = is_cutscene ? g_Config.audio.cutscene_volume
        : is_ambient                       ? g_Config.audio.ambient_volume
                                           : g_Config.audio.music_volume;
    const double multiplier = !is_underwater || is_cutscene ? 1.0
        : is_ambient ? g_Config.audio.underwater_ambient_volume
                     : g_Config.audio.underwater_music_volume;
    Music_SetVolume(base_volume * multiplier);
}

void Camera_RefreshFromTrigger(const TRIGGER *const trigger)
{
    int16_t target_ok = 2;

    const TRIGGER_CMD *cmd = trigger->command;
    for (; cmd != nullptr; cmd = cmd->next_cmd) {
        if (cmd->type == TO_CAMERA) {
            const TRIGGER_CAMERA_DATA *const cam_data =
                (TRIGGER_CAMERA_DATA *)cmd->parameter;
            if (cam_data->camera_num == g_Camera.last) {
                g_Camera.num = cam_data->camera_num;

                if (g_Camera.timer < 0 || g_Camera.type == CAM_LOOK
                    || g_Camera.type == CAM_COMBAT) {
                    g_Camera.timer = -1;
                    target_ok = 0;
                } else {
                    g_Camera.type = CAM_FIXED;
                    if (g_Config.visuals.fix_glide_cameras
                        && cam_data->glide != 0) {
                        g_Camera.speed = cam_data->glide + 1;
                    }
                    target_ok = 1;
                }
            } else {
                target_ok = 0;
            }
        } else if (cmd->type == TO_TARGET) {
            if (g_Camera.type != CAM_LOOK && g_Camera.type != CAM_COMBAT) {
                g_Camera.item = Item_Get((int16_t)(intptr_t)cmd->parameter);
            }
        }
    }

    if (g_Camera.item != nullptr
        && (target_ok == 0
            || (target_ok == 2 && g_Camera.item->looked_at
                && g_Camera.item != g_Camera.last_item))) {
        g_Camera.item = nullptr;
    }

    if (g_Config.visuals.camera_mode != CAMERA_MODE_TR1 && g_Camera.num == -1
        && g_Camera.timer > 0) {
        g_Camera.timer = -1;
    }
}

void Camera_EnsureEnvironment(void)
{
    if (g_Camera.mic_pos.room_num != NO_ROOM) {
        const ROOM *const room = Room_Get(g_Camera.mic_pos.room_num);
        if (room->flags.underwater) {
            M_AdjustMusicVolume(true);
            Sound_Effect(SFX_UNDERWATER, nullptr, SPM_ALWAYS);
        } else {
            M_AdjustMusicVolume(false);
            Sound_StopEffect(SFX_UNDERWATER);
        }
    }

    if (g_Camera.pos.room_num != NO_ROOM) {
        const ROOM *const room = Room_Get(g_Camera.pos.room_num);
        g_Camera.underwater = room->flags.underwater;
    }
}

void Camera_UpdateMicPosition(void)
{
    if (g_Config.audio.enable_lara_mic) {
        const LARA_INFO *const lara_info = Lara_GetLaraInfo();
        const ITEM *const lara_item = Lara_GetItem();
        g_Camera.actual_angle =
            lara_info->torso_rot.y + lara_info->head_rot.y + lara_item->rot.y;
        g_Camera.mic_pos.pos = lara_item->pos;
        g_Camera.mic_pos.room_num = lara_item->room_num;
    } else {
        g_Camera.actual_angle = Math_Atan(
            g_Camera.target.z - g_Camera.pos.z,
            g_Camera.target.x - g_Camera.pos.x);
        if (g_TRVersion == 1) {
            g_Camera.mic_pos = g_Camera.pos;
        } else {
            g_Camera.mic_pos.pos.x = g_Camera.pos.x
                + ((g_PhdPersp * Math_Sin(g_Camera.actual_angle)) >> W2V_SHIFT);
            g_Camera.mic_pos.pos.z = g_Camera.pos.z
                + ((g_PhdPersp * Math_Cos(g_Camera.actual_angle)) >> W2V_SHIFT);
            g_Camera.mic_pos.pos.y = g_Camera.pos.y;
            g_Camera.mic_pos.room_num = g_Camera.pos.room_num;
        }
    }
}
