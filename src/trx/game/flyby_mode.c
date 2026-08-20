#include <trx/game/flyby_mode.h>

#include <trx/config.h>
#include <trx/game/camera.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>

static struct {
    int32_t lara_health;
    int32_t lara_air;
} m_Priv = {};

static void M_CacheLaraInfo(void)
{
    m_Priv.lara_health = Lara_GetItem()->hit_points;
    m_Priv.lara_air = Lara_GetLaraInfo()->air;
}

static void M_RestoreLaraInfo(void)
{
    Lara_GetItem()->hit_points = m_Priv.lara_health;
    Lara_GetLaraInfo()->air = m_Priv.lara_air;
}

bool FlybyMode_Activate(const int32_t sequence_idx, const bool one_shot)
{
    if (!Camera_FlybyMode_Activate(sequence_idx, one_shot)) {
        return false;
    }
    M_CacheLaraInfo();
    return true;
}

void FlybyMode_Deactivate(void)
{
    Camera_FlybyMode_Deactivate();
}

void FlybyMode_Stop(void)
{
    Camera_FlybyMode_Reset();
    Lara_SetControllable(true);
}

bool FlybyMode_IsActive(void)
{
    return Camera_FlybyMode_IsActive();
}

bool FlybyMode_Cancel(const bool force)
{
    return Camera_Flybymode_Cancel(force);
}

void FlybyMode_PreControl(void)
{
    if (!FlybyMode_IsActive()) {
        return;
    }

    if (g_InputDB.fly_cheat && g_Config.gameplay.enable_cheats) {
        FlybyMode_Cancel(true);
        return;
    }

    const bool skips_enabled = g_Config.gameplay.enable_cinematic_skips;
    const bool skip_requested =
        g_InputDB.look || (g_InputDB.option && skips_enabled);
    if (skip_requested && FlybyMode_Cancel(skips_enabled)) {
        g_InputDB.look = false;
        g_InputDB.option = false;
        Input_HoldOffSkip();
        return;
    }

    if (!Lara_IsControllable()) {
        InputState_Clear(&g_Input);
        InputState_Clear(&g_InputDB);
    } else {
        g_Input.look = false;
        g_InputDB.look = false;
        g_Input.camera_reset = false;
        g_InputDB.camera_reset = false;
    }
}

void FlybyMode_PostControl(void)
{
    if (!FlybyMode_IsActive()) {
        return;
    }

    if (Camera_Binoculars_IsActive()) {
        FlybyMode_Cancel(true);
        return;
    }

    g_Camera.type = CAM_FLYBY_MODE;
    M_RestoreLaraInfo();
}
