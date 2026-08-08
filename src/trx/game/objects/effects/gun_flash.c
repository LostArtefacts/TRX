#include <trx/config.h>
#include <trx/core/colors.h>
#include <trx/game/effects.h>
#include <trx/game/gun/misc.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/version.h>

static void M_Control(const int16_t effect_num)
{
    EFFECT *const effect = Effect_Get(effect_num);

    effect->counter--;
    if (effect->counter == 0) {
        Effect_Destroy(effect_num);
        return;
    }

    effect->rot.z = Random_GetControl();
    if (g_Config.visuals.enable_gun_lighting) {
        if (g_TRVersion >= 3) {
            Output_AddDynamicLightRGB(
                effect->pos, 12, (RGB_888) { 192, 144, 0 });
        } else {
            Output_AddDynamicLight(effect->pos, 12, 11);
        }
    }
}

// Returning false leaves the flash mesh itself to the generic effect drawing.
static bool M_Draw(const EFFECT *const effect)
{
    if (g_TRVersion < 3 && !g_Config.visuals.enable_gun_glow) {
        return false;
    }

    const OBJECT *const glow_obj = Object_Get(O_GLOW);
    if (!glow_obj->loaded) {
        return false;
    }

    const bool is_tr3 = g_TRVersion >= 3;
    const RGBA_F tint = is_tr3 ? (RGBA_F) { 1.0f, 0.89f, 0.13f, 1.0f }
                               : (RGBA_F) { 0.247f, 0.22f, 0.059f, 1.0f };
    Output_DrawSprite(
        effect->interp.result.pos.x, effect->interp.result.pos.y,
        effect->interp.result.pos.z, glow_obj->mesh_idx,
        is_tr3 ? SHADE_NEUTRAL : 0, tint, DRAW_BLEND_ADD, is_tr3 ? 1.0f : 2.0f);
    return false;
}

static void M_Setup(OBJECT *const obj)
{
    obj->effect_control_func = M_Control;
    obj->effect_draw_func = M_Draw;
    Gun_ApplyFlashSemiTransparency();
}

REGISTER_OBJECT(O_GUN_FLASH, M_Setup)
