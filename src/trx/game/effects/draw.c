#include <trx/game/effects/draw.h>

#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/draw.h>
#include <trx/game/objects/families.h>
#include <trx/game/output.h>

static void M_DrawSprite(const EFFECT *const effect)
{
    const OBJECT *const obj = Object_Get(effect->object_id);
    const RGBA_F tint =
        ObjectFamily_Has(effect->object_id, OBJ_FAMILY_WATER_SPRITE)
        ? COLOR_RGBA_F_WHITE
        : Output_GetTint();
    int16_t shade = effect->shade;
    if (shade == -1) {
        Output_CalculateLight(effect->pos, effect->room_num);
        shade = Output_GetLightAdder();
    }
    Output_DrawSprite(
        effect->interp.result.pos.x, effect->interp.result.pos.y,
        effect->interp.result.pos.z, obj->mesh_idx - effect->frame_num, shade,
        tint, Output_GetSpriteDrawType(obj), 1.0f);
}

static void M_DrawMesh(const EFFECT *const effect)
{
    const OBJECT *const obj = Object_Get(effect->object_id);
    Matrix_Push();
    Matrix_TranslateAbs32(effect->interp.result.pos);
    Matrix_Rot16(effect->interp.result.rot);
    if (obj->mesh_count != 0) {
        Output_CalculateStaticLight(effect->shade);
        Object_DrawMesh(obj->mesh_idx, -1, false);
    } else {
        Output_CalculateLight(effect->interp.result.pos, effect->room_num);
        Object_DrawMesh(effect->frame_num, -1, false);
    }
    Matrix_Pop();
}

bool Effect_Draw(const EFFECT *const effect)
{
    if (Object_Get(effect->object_id)->mesh_count < 0) {
        M_DrawSprite(effect);
    } else {
        M_DrawMesh(effect);
    }
    return true;
}
