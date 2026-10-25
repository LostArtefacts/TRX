#include <trx/game/effects/draw.h>

#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/draw.h>
#include <trx/game/objects/families.h>
#include <trx/game/output.h>

static void M_DrawSprite(
    const EFFECT *const effect, const EFFECT_DRAW_DESC *const desc)
{
    const OBJECT *const obj = Object_Get(effect->object_id);
    const RGBA_F tint =
        ObjectFamily_Has(effect->object_id, OBJ_FAMILY_WATER_SPRITE)
        ? COLOR_RGBA_F_WHITE
        : Output_GetTint();
    int16_t shade = effect->shade;
    if (shade == -1 || desc->room_light) {
        Output_CalculateLight(effect->pos, effect->room_num);
        Output_BrightenLight(desc->brightness);
        shade = Output_GetLightAdder();
    }
    Output_DrawSprite(
        effect->interp.result.pos.x, effect->interp.result.pos.y,
        effect->interp.result.pos.z, obj->mesh_idx - effect->frame_num, shade,
        tint, Output_GetSpriteDrawType(obj), 1.0f);
}

static void M_DrawMesh(
    const EFFECT *const effect, const EFFECT_DRAW_DESC *const desc)
{
    const OBJECT *const obj = Object_Get(effect->object_id);
    Matrix_Push();
    Matrix_TranslateAbs32(effect->interp.result.pos);
    Matrix_Rot16(effect->interp.result.rot);
    if (obj->mesh_count != 0 && !desc->room_light) {
        Output_CalculateStaticLight(effect->shade);
    } else {
        Output_CalculateLight(effect->interp.result.pos, effect->room_num);
    }
    Output_BrightenLight(desc->brightness);
    Object_DrawMesh(
        obj->mesh_count != 0 ? obj->mesh_idx : effect->frame_num, -1, false);
    Matrix_Pop();
}

bool Effect_Draw(const EFFECT *const effect)
{
    return Effect_DrawEx(effect, &(EFFECT_DRAW_DESC) {});
}

bool Effect_DrawEx(
    const EFFECT *const effect, const EFFECT_DRAW_DESC *const desc)
{
    if (Object_Get(effect->object_id)->mesh_count < 0) {
        M_DrawSprite(effect, desc);
    } else {
        M_DrawMesh(effect, desc);
    }
    return true;
}
