#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/flyby_mode.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

static void M_HandleCamera(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const TRIGGER_CAMERA_DATA *const cam_data =
        (TRIGGER_CAMERA_DATA *)cmd->parameter;
    OBJECT_VECTOR *const camera = Camera_GetFixedObject(cam_data->camera_num);
    if ((camera->flags & IF_ONE_SHOT) != 0) {
        return;
    }

    g_Camera.num = cam_data->camera_num;

    if ((g_Camera.type == CAM_LOOK || g_Camera.type == CAM_COMBAT)
        && !Camera_IsLocked(g_Camera.num)) {
        return;
    }

    if (trigger->type == TT_COMBAT) {
        return;
    }

    if (trigger->type == TT_SWITCH && trigger->timer != 0
        && status->switch_off) {
        return;
    }

    if (g_Camera.num == g_Camera.last && trigger->type != TT_SWITCH) {
        return;
    }

    g_Camera.timer = LOGIC_FPS * cam_data->timer;

    if (cam_data->one_shot) {
        camera->flags |= IF_ONE_SHOT;
    }

    g_Camera.speed = 1;
    if (g_Config.visuals.enable_glide_cameras) {
        g_Camera.speed += cam_data->glide;
    }
    g_Camera.type = status->is_heavy ? CAM_HEAVY : CAM_FIXED;
}

static void M_HandleTarget(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    const int16_t target_num = (int16_t)(intptr_t)cmd->parameter;
    status->camera_item = Item_Get(target_num);
}

static void M_HandleSink(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (g_TRVersion >= 3) {
        lara->current.active = 1 + (int16_t)(intptr_t)cmd->parameter;
    } else {
        const OBJECT_VECTOR *const sink =
            Camera_GetFixedObject((int16_t)(intptr_t)cmd->parameter);

        if (g_TRVersion == 2 || lara->lot.required_box != sink->flags) {
            lara->lot.target = sink->pos;
            lara->lot.required_box = sink->flags;
        }
        lara->current.active = sink->data * 6;
    }
}

static void M_HandleFlyby(
    const TRIGGER *const trigger, const TRIGGER_CMD *const cmd,
    TRIGGER_STATUS *const status)
{
    if (Room_IsAntiTrigger(trigger->type)) {
        FlybyMode_Deactivate();
        return;
    }

    const TRIGGER_FLYBY_DATA *const flyby_data =
        (TRIGGER_FLYBY_DATA *)cmd->parameter;
    FlybyMode_Activate(flyby_data->sequence_num, flyby_data->one_shot);
}

REGISTER_TRIGGER_HANDLER(TO_CAMERA, M_HandleCamera)
REGISTER_TRIGGER_HANDLER(TO_TARGET, M_HandleTarget)
REGISTER_TRIGGER_HANDLER(TO_SINK, M_HandleSink)
REGISTER_TRIGGER_HANDLER(TO_FLYBY, M_HandleFlyby)
