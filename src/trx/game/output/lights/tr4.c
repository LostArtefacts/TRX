// TR4 lighting family: port of the OG per-item colored light pipeline
// (specific/lighting.cpp). Room lights are baked per level into a compact
// table (ProcessRoomData port); each item keeps current/previous light lists
// with 8-frame fades (CreateLightList/FadeLightList/CalcAmbientLight ports);
// each staged mesh instance resolves the lists into a flat view-space light
// list (SetupLight port) consumed by the vertex shader.

#include <trx/core/colors.h>
#include <trx/core/math/geom.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/game/camera/vars.h>
#include <trx/game/const.h>
#include <trx/game/items/manager.h>
#include <trx/game/level/settings.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/vars.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/fog_bulbs.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/random.h>
#include <trx/gl/utils.h>
#include <trx/version.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define M_MAX_ITEM_LIGHTS 21 // the OG ITEM_LIGHT current/prev list capacity
// Enough for a cutscene's cast, which is what asks to be lit without living
// in the item pool.
#define M_MAX_LOOSE_LIGHTS 16
#define M_FADE_FRAMES 8

// Room light baked at level load (the OG PCLIGHT_INFO, drawroom.cpp
// ProcessRoomData).
typedef struct {
    XYZ_F pos;
    XYZ_32 ipos;
    float r, g, b; // color 0..1, premultiplied by |intensity| / 8191
    int32_t shadow; // |intensity| / 8191 * 255 (SHADOW lights)
    float inner, outer; // radii; for SPOT: raw cosines of half angles
    float cutoff; // SPOT range
    float nx, ny, nz; // negated direction (SUN/SPOT)
    LIGHT_TYPE type;
} M_BAKED_LIGHT;

typedef struct {
    M_BAKED_LIGHT *lights;
    int32_t count;
} M_ROOM_BAKE;

// Per-item working copy of a room light with fade state (the OG PCLIGHT).
typedef struct {
    XYZ_F pos;
    XYZ_32 ipos;
    float r, g, b; // current faded color
    float tr, tg, tb; // fade target color
    float rs, gs, bs; // per-frame fade step
    int32_t fcnt;
    int32_t shadow; // current shadow amount (base << 3 when fully active)
    int32_t shadow_base; // the OG PCLIGHT.inx (for SHADOW lights)
    int32_t shadow_step; // the OG PCLIGHT.iny
    float inner, outer;
    float cutoff;
    float nx, ny, nz;
    int64_t range; // squared distance from the item sample pos
    LIGHT_TYPE type;
    bool active;
} M_ITEM_ROOM_LIGHT;

// Per-item light state (the OG ITEM_LIGHT).
typedef struct {
    RGB_888 ambient;
    int32_t amb_ch[3]; // <<3 fixed-point channels during the ambient fade
    int32_t amb_step[3];
    int32_t amb_fcnt; // -1 = snap on next update
    int16_t light_room_num; // room the light lists were built for
    XYZ_32 item_pos;
    M_ITEM_ROOM_LIGHT lights[2][M_MAX_ITEM_LIGHTS];
    int32_t n_lights[2];
    int32_t cur; // index of the current list; 1 - cur = previous
    bool used;
} M_ITEM_LIGHT;

// Fully resolved per-mesh-instance light list handed to the GPU.
typedef struct {
    float ambient[3]; // 0..255 scale (128 = OG neutral)
    int32_t count;
    struct {
        XYZ_F vec; // view space, attenuation-scaled (the OG convention)
        float rad;
        float r, g, b; // 0..1
    } lights[OUTPUT_TR4_MAX_STAGED_LIGHTS];
} M_STAGED_LIST;

static M_ROOM_BAKE *m_RoomBake = nullptr;
static int32_t m_RoomBakeCount = 0;

static M_ITEM_LIGHT m_ItemLights[MAX_ITEMS] = {};
static M_ITEM_LIGHT m_ScratchLight = {}; // the OG StaticMeshLightItem
static const ITEM *m_LooseLightKeys[M_MAX_LOOSE_LIGHTS] = {};
static M_ITEM_LIGHT m_LooseLights[M_MAX_LOOSE_LIGHTS] = {};
static M_ITEM_LIGHT *m_CurrentItemLight = nullptr;
static bool m_CurrentIsPickup = false; // the OG SetupLight_thing
static bool m_CurrentHasList = false;
static bool m_InventoryMode = false;

static VECTOR *m_StagedPool = nullptr; // of M_STAGED_LIST
static int32_t m_PoolGeneration = 0;

static struct {
    int32_t generation;
    int32_t handle;
    RGB_F constant;
    RGB_F own_ambient;
    OUTPUT_LS_MODE mode;
} m_UploadCache = { .handle = -2 };

static XYZ_F M_ViewRotate(const XYZ_F v)
{
    const MATRIX *const m = &g_ViewMatrix;
    const float s = 1.0f / (float)(1 << W2V_SHIFT);
    return (XYZ_F) {
        .x = (m->_00 * v.x + m->_01 * v.y + m->_02 * v.z) * s,
        .y = (m->_10 * v.x + m->_11 * v.y + m->_12 * v.z) * s,
        .z = (m->_20 * v.x + m->_21 * v.y + m->_22 * v.z) * s,
    };
}

static void M_SetConstantLight(const RGB_F ambient)
{
    const RGB_F colors[3] = {};
    const XYZ_32 dirs_view[3] = {};
    Output_SetTR3Light(ambient, colors, dirs_view);
    m_CurrentHasList = false;
}

static void M_SetScalarFallback(const RGB_888 ambient)
{
    // Keep the legacy scalar shade meaningful for sprite/effect code paths.
    // 128 is the OG neutral brightness.
    const float avg = (ambient.r + ambient.g + ambient.b) / 3.0f;
    Output_SetLightDivider(0);
    Output_SetLightAdder(Output_Lights_ShadeFromMul(avg / 128.0f));
}

static void M_FreeRoomBake(void)
{
    if (m_RoomBake != nullptr) {
        for (int32_t i = 0; i < m_RoomBakeCount; i++) {
            Memory_FreePointer(&m_RoomBake[i].lights);
        }
        Memory_FreePointer(&m_RoomBake);
    }
    m_RoomBakeCount = 0;
    Output_FogBulbs_ResetStatic();
}

// Port of the OG ProcessRoomData (drawroom.cpp): converts parsed room lights
// into the baked table and extracts fog bulbs.
static void M_BakeRoomLights(void)
{
    M_FreeRoomBake();
    m_RoomBakeCount = Room_GetCount();
    m_RoomBake = Memory_Alloc(sizeof(M_ROOM_BAKE) * m_RoomBakeCount);

    for (int32_t room_num = 0; room_num < m_RoomBakeCount; room_num++) {
        const ROOM *const room = Room_Get(room_num);
        M_ROOM_BAKE *const bake = &m_RoomBake[room_num];
        if (room->num_lights == 0) {
            continue;
        }

        bake->lights = Memory_Alloc(sizeof(M_BAKED_LIGHT) * room->num_lights);
        int32_t n_lights = 0;

        for (int32_t i = 0; i < room->num_lights; i++) {
            const LIGHT *const light = &room->lights[i];
            if (light->layout != LIGHT_LAYOUT_TR4) {
                continue;
            }

            if (light->type == LIGHT_TYPE_FOG_BULB) {
                // A fog bulb carries no color of its own: the level data
                // holds its density in the red channel and leaves the other
                // two alone. The OG draws the bulb in the fog color, which a
                // bulb follows until a script gives it a color.
                Output_FogBulbs_AddStatic(
                    light->pos, light->u.tr4.outer_radius, light->color.r,
                    (int16_t)room_num);
                continue;
            }

            if (light->color.r == 0 && light->color.g == 0
                && light->color.b == 0 && light->type == LIGHT_TYPE_SPOT) {
                continue;
            }

            // NOTE: the OG reads intensity/type of light[n_lights] instead of
            // light[i] here (drawroom.cpp:472-484), so any skipped light
            // desyncs them. Kept for faithfulness — levels were authored
            // against this behavior.
            const LIGHT *const quirk_light = &room->lights[n_lights];
            float intensity = ABS(quirk_light->u.tr4.intensity) / 8191.0f;

            M_BAKED_LIGHT *const baked = &bake->lights[n_lights];
            baked->r = light->color.r * (1.0f / 255.0f) * intensity;
            baked->g = light->color.g * (1.0f / 255.0f) * intensity;
            baked->b = light->color.b * (1.0f / 255.0f) * intensity;
            if (quirk_light->type != LIGHT_TYPE_SUN) {
                baked->shadow = (int32_t)(intensity * 255.0f);
            }
            baked->pos = (XYZ_F) {
                (float)light->pos.x,
                (float)light->pos.y,
                (float)light->pos.z,
            };
            baked->ipos = light->pos;
            baked->nx = -light->u.tr4.dir.x;
            baked->ny = -light->u.tr4.dir.y;
            baked->nz = -light->u.tr4.dir.z;
            baked->inner = light->u.tr4.inner_radius;
            baked->outer = light->u.tr4.outer_radius;
            baked->cutoff = light->u.tr4.cutoff;
            baked->type = light->type;
            n_lights++;
        }
        bake->count = n_lights;
    }
}

// Port of the OG CalcAmbientLight (lighting.cpp:496).
static void M_CalcAmbientLight(M_ITEM_LIGHT *const il, const RGB_888 target)
{
    if (il->ambient.r == target.r && il->ambient.g == target.g
        && il->ambient.b == target.b) {
        return;
    }

    if (il->amb_fcnt == -1) {
        il->ambient = target;
        il->amb_fcnt = 0;
        return;
    }

    if (il->amb_fcnt == 0) {
        const int32_t cur[3] = { il->ambient.r, il->ambient.g, il->ambient.b };
        const int32_t dst[3] = { target.r, target.g, target.b };
        for (int32_t i = 0; i < 3; i++) {
            il->amb_step[i] = dst[i] - cur[i];
            il->amb_ch[i] = cur[i] << 3;
        }
        il->amb_fcnt = M_FADE_FRAMES;
    }

    if (il->amb_fcnt != 0) {
        for (int32_t i = 0; i < 3; i++) {
            il->amb_ch[i] += il->amb_step[i];
        }
        il->ambient = (RGB_888) {
            .r = il->amb_ch[0] >> 3,
            .g = il->amb_ch[1] >> 3,
            .b = il->amb_ch[2] >> 3,
        };
        il->amb_fcnt--;
    }
}

// Port of the OG FadeLightList (lighting.cpp:415).
static void M_FadeLightList(M_ITEM_ROOM_LIGHT *const lights, const int32_t n)
{
    for (int32_t i = 0; i < n; i++) {
        M_ITEM_ROOM_LIGHT *const light = &lights[i];
        if (!light->active || light->fcnt == 0) {
            continue;
        }

        if (light->type == LIGHT_TYPE_SHADOW) {
            light->shadow += light->shadow_step;
        } else {
            light->r += light->rs;
            light->g += light->gs;
            light->b += light->bs;
        }

        light->fcnt--;

        if (light->type == LIGHT_TYPE_SHADOW) {
            if (light->shadow <= 0) {
                light->active = false;
            }
        } else if (light->r <= 0.0f && light->g <= 0.0f && light->b <= 0.0f) {
            light->active = false;
        }
    }
}

static void M_ArmFadeOut(M_ITEM_ROOM_LIGHT *const light)
{
    if (light->type == LIGHT_TYPE_SHADOW) {
        light->shadow_step = -light->shadow >> 3;
    } else {
        light->rs = light->r * -0.125f;
        light->gs = light->g * -0.125f;
        light->bs = light->b * -0.125f;
    }
    light->fcnt = M_FADE_FRAMES;
}

// Port of the OG CreateLightList (lighting.cpp:243).
static void M_CreateLightList(M_ITEM_LIGHT *const il, const int16_t room_num)
{
    if (il->light_room_num != room_num) {
        il->cur = 1 - il->cur;
        il->light_room_num = room_num;

        M_ITEM_ROOM_LIGHT *const prev = il->lights[1 - il->cur];
        for (int32_t i = 0; i < il->n_lights[1 - il->cur]; i++) {
            if (prev[i].active) {
                if (prev[i].type != LIGHT_TYPE_SHADOW) {
                    prev[i].tr = 0.0f;
                    prev[i].tg = 0.0f;
                    prev[i].tb = 0.0f;
                }
                M_ArmFadeOut(&prev[i]);
            }
        }

        const M_ROOM_BAKE *const bake = &m_RoomBake[room_num];
        const int32_t count = MIN(bake->count, M_MAX_ITEM_LIGHTS);
        il->n_lights[il->cur] = count;
        M_ITEM_ROOM_LIGHT *const cur = il->lights[il->cur];
        for (int32_t i = 0; i < count; i++) {
            const M_BAKED_LIGHT *const baked = &bake->lights[i];
            cur[i] = (M_ITEM_ROOM_LIGHT) {
                .pos = baked->pos,
                .ipos = baked->ipos,
                .r = baked->r,
                .g = baked->g,
                .b = baked->b,
                .tr = baked->r,
                .tg = baked->g,
                .tb = baked->b,
                .shadow = baked->shadow << 3,
                .shadow_base = baked->shadow,
                .inner = baked->inner,
                .outer = baked->outer,
                .cutoff = baked->cutoff,
                .nx = baked->nx,
                .ny = baked->ny,
                .nz = baked->nz,
                .type = baked->type,
                .active = false,
            };
        }
    }

    M_ITEM_ROOM_LIGHT *const cur = il->lights[il->cur];
    for (int32_t i = 0; i < il->n_lights[il->cur]; i++) {
        M_ITEM_ROOM_LIGHT *const light = &cur[i];
        bool in_range = true;

        const int64_t dx = light->ipos.x - il->item_pos.x;
        const int64_t dy = light->ipos.y - il->item_pos.y;
        const int64_t dz = light->ipos.z - il->item_pos.z;
        const int64_t range = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);

        if (light->type == LIGHT_TYPE_POINT
            || light->type == LIGHT_TYPE_SHADOW) {
            if (range > (int64_t)SQUARE((double)light->outer)) {
                in_range = false;
            }
        } else if (light->type == LIGHT_TYPE_SPOT) {
            if (range > (int64_t)SQUARE((double)light->cutoff)) {
                in_range = false;
            } else {
                XYZ_F vec = { (float)-dx, (float)-dy, (float)-dz };
                const float len =
                    sqrtf(SQUARE(vec.x) + SQUARE(vec.y) + SQUARE(vec.z));
                if (len > 0.0f) {
                    vec.x /= len;
                    vec.y /= len;
                    vec.z /= len;
                }
                if (light->nx * vec.x + light->ny * vec.y + light->nz * vec.z
                    <= light->outer) {
                    in_range = false;
                }
            }
        }

        light->range = range;

        if (in_range) {
            if (!light->active) {
                if (light->type == LIGHT_TYPE_SHADOW) {
                    light->shadow_step = light->shadow_base;
                    light->shadow = 0;
                } else {
                    light->rs = light->tr * 0.125f;
                    light->gs = light->tg * 0.125f;
                    light->bs = light->tb * 0.125f;
                    light->r = 0.0f;
                    light->g = 0.0f;
                    light->b = 0.0f;
                }
                light->active = true;
                light->fcnt = M_FADE_FRAMES;
            }
        } else if (light->active && light->fcnt == 0) {
            M_ArmFadeOut(light);
        }
    }

    M_FadeLightList(cur, il->n_lights[il->cur]);
    M_FadeLightList(il->lights[1 - il->cur], il->n_lights[1 - il->cur]);
}

// Port of the OG SetupLight (lighting.cpp:85). Emits into `list`; the shadow
// lights only darken the ambient.
static void M_StageLight(
    const M_ITEM_ROOM_LIGHT *const light, const XYZ_F mesh_pos,
    M_STAGED_LIST *const list)
{
    if (list->count >= OUTPUT_TR4_MAX_STAGED_LIGHTS
        && light->type != LIGHT_TYPE_SHADOW) {
        return;
    }

    switch (light->type) {
    case LIGHT_TYPE_SUN: {
        const float len =
            sqrtf(SQUARE(light->nx) + SQUARE(light->ny) + SQUARE(light->nz));
        if (len <= 0.0f) {
            break;
        }
        const float num = -1.0f / len;
        const float mul = m_InventoryMode ? 2.0f : 0.75f;
        const XYZ_F vec = M_ViewRotate((XYZ_F) {
            light->nx * num,
            light->ny * num,
            light->nz * num,
        });
        list->lights[list->count++] = (typeof(list->lights[0])) {
            .vec = vec,
            .rad = 1.0f,
            // Premultiplied so the shader needs no sun-specific branch:
            // the OG scales the sun dot by 0.75 in-game and 2.0 in inventory.
            .r = light->r * mul,
            .g = light->g * mul,
            .b = light->b * mul,
        };
        break;
    }

    case LIGHT_TYPE_POINT:
    case LIGHT_TYPE_SPOT: {
        float x = light->pos.x - mesh_pos.x;
        float y = light->pos.y - mesh_pos.y;
        float z = light->pos.z - mesh_pos.z;
        if (x == 0.0f || y == 0.0f || z == 0.0f) {
            // Avoid degenerate vectors when the light shares an axis plane
            // with the mesh origin (the OG does the same).
            x += 1.0f;
            y += 1.0f;
            z += 1.0f;
        }

        const float dist = sqrtf(SQUARE(x) + SQUARE(y) + SQUARE(z));
        float rad;
        float scale;
        if (light->type == LIGHT_TYPE_POINT) {
            scale = 2.0f / dist;
            rad = (light->outer - dist) / light->outer;
        } else {
            scale = (m_CurrentIsPickup ? 2.0f : 1.0f) / dist;
            // The cone test already happened in the light list; from here
            // spots shade exactly like point lights.
            rad = 1.0f - dist / light->cutoff;
        }
        CLAMPL(rad, 0.0f);

        if (light->type == LIGHT_TYPE_POINT && m_CurrentIsPickup
            && rad < 1.0f) {
            // Pickups fold nearby point lights into the ambient so they
            // glow; halve the radius to avoid double-counting the color.
            for (int32_t i = 0; i < 3; i++) {
                const float c =
                    i == 0 ? light->r : (i == 1 ? light->g : light->b);
                list->ambient[i] += rad * c * 255.0f;
                CLAMPG(list->ambient[i], 255.0f);
            }
            rad /= 2.0f;
        }

        const XYZ_F vec =
            M_ViewRotate((XYZ_F) { x * scale, y * scale, z * scale });
        list->lights[list->count++] = (typeof(list->lights[0])) {
            .vec = vec,
            .rad = rad,
            .r = light->r,
            .g = light->g,
            .b = light->b,
        };
        break;
    }

    case LIGHT_TYPE_SHADOW: {
        const float val = sqrtf((float)light->range);
        int32_t val2 = light->shadow >> 3;
        if (val >= light->inner) {
            val2 = (int32_t)((val - light->outer)
                             / ((light->outer - light->inner) / (float)-val2));
        }
        CLAMPL(val2, 0);
        val2 >>= 1;
        for (int32_t i = 0; i < 3; i++) {
            list->ambient[i] -= (float)val2;
            CLAMPL(list->ambient[i], 0.0f);
        }
        break;
    }

    default:
        break;
    }
}

// Port of the OG SetupDynamicLight (lighting.cpp:58).
static void M_StageDynamicLights(
    const XYZ_F mesh_pos, M_STAGED_LIST *const list)
{
    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        if (list->count >= OUTPUT_TR4_MAX_STAGED_LIGHTS) {
            break;
        }
        const OUTPUT_DYNAMIC_LIGHT *const entry = Vector_Get(dynamic_lights, i);
        const LIGHT *const light = &entry->light;
        const int32_t raw_falloff = light->u.legacy.falloff.value_1;
        const float falloff = (raw_falloff >> 1) + (raw_falloff >> 3);

        const float x = light->pos.x - mesh_pos.x;
        const float y = light->pos.y - mesh_pos.y;
        const float z = light->pos.z - mesh_pos.z;
        const float dist = sqrtf(SQUARE(x) + SQUARE(y) + SQUARE(z));
        if (dist > falloff || falloff <= 0.0f) {
            continue;
        }

        const float scale = 1.0f / dist;
        const XYZ_F vec =
            M_ViewRotate((XYZ_F) { x * scale, y * scale, z * scale });
        list->lights[list->count++] = (typeof(list->lights[0])) {
            .vec = vec,
            .rad = (falloff - dist) / falloff,
            .r = light->color.r / 255.0f,
            .g = light->color.g / 255.0f,
            .b = light->color.b / 255.0f,
        };
    }
}

// An item outside the pool - a cutscene actor is posed from a track rather than
// living in a room - has no slot of its own, and the scratch it would otherwise
// fall to is shared with every effect and static drawn that frame. This state
// fades between frames, so sharing it makes an actor's light jump with the
// effects and statics drawn beside it. The original engine keeps it on the
// item, so one is handed out per item here instead.
static M_ITEM_LIGHT *M_GetLooseItemLight(const ITEM *const item)
{
    for (int32_t i = 0; i < M_MAX_LOOSE_LIGHTS; i++) {
        if (m_LooseLightKeys[i] == item) {
            return &m_LooseLights[i];
        }
    }
    for (int32_t i = 0; i < M_MAX_LOOSE_LIGHTS; i++) {
        if (m_LooseLightKeys[i] == nullptr) {
            m_LooseLightKeys[i] = item;
            m_LooseLights[i] = (M_ITEM_LIGHT) {};
            return &m_LooseLights[i];
        }
    }
    return &m_ScratchLight;
}

static M_ITEM_LIGHT *M_GetItemLight(const ITEM *const item)
{
    if (item == nullptr) {
        return &m_ScratchLight;
    }
    // Measured against the pool rather than through Item_GetIndex, whose
    // int16_t result would wrap a stray pointer into a real item's slot.
    const ITEM *const pool = Item_Get(0);
    if (pool != nullptr && item >= pool && item < pool + MAX_ITEMS) {
        return &m_ItemLights[item - pool];
    }
    return M_GetLooseItemLight(item);
}

static void M_PrepareItemLight(M_ITEM_LIGHT *const il)
{
    if (!il->used) {
        il->used = true;
        il->amb_fcnt = -1; // snap the first ambient update
        il->light_room_num = NO_ROOM;
        il->n_lights[0] = 0;
        il->n_lights[1] = 0;
    }
}

static void M_CalculateObjectLightingAt(
    const ITEM *const item, const GAME_VECTOR sample_pos)
{
    M_ITEM_LIGHT *const il = M_GetItemLight(item);
    M_PrepareItemLight(il);
    il->item_pos = sample_pos.pos;

    int16_t room_num = sample_pos.room_num;
    Room_GetSector(sample_pos.pos, &room_num);
    M_CalcAmbientLight(il, Room_Get(room_num)->ambient_rgb);

    M_CreateLightList(il, item != nullptr ? item->room_num : room_num);

    m_CurrentItemLight = il;
    m_CurrentIsPickup =
        item != nullptr && Object_IsType(item->object_id, g_PickupObjects);
    m_CurrentHasList = true;

    M_SetScalarFallback(il->ambient);
}

// For effects and other list-less callers: ambient only, like the OG
// StaticMeshLightItem (its light lists are never populated).
static void M_CalculateLight(const XYZ_32 pos, const int16_t room_num)
{
    M_ITEM_LIGHT *const il = &m_ScratchLight;
    M_PrepareItemLight(il);
    il->item_pos = pos;
    il->ambient = Room_Get(room_num)->ambient_rgb;
    il->amb_fcnt = 0;
    il->n_lights[0] = 0;
    il->n_lights[1] = 0;
    il->light_room_num = NO_ROOM;

    m_CurrentItemLight = il;
    m_CurrentIsPickup = false;
    m_CurrentHasList = true;

    M_SetScalarFallback(il->ambient);
}

static void M_FillInstanceLight(
    OUTPUT_LIGHT_INFO *const info, const MATRIX *const wmatrix)
{
    info->tr4.handle = -1;
    if (!m_CurrentHasList || m_CurrentItemLight == nullptr) {
        return;
    }

    const M_ITEM_LIGHT *const il = m_CurrentItemLight;
    const float inv_scale = 1.0f / (float)(1 << W2V_SHIFT);
    const XYZ_F mesh_pos = {
        wmatrix->_03 * inv_scale,
        wmatrix->_13 * inv_scale,
        wmatrix->_23 * inv_scale,
    };

    M_STAGED_LIST list = {
        .ambient = { il->ambient.r, il->ambient.g, il->ambient.b },
        .count = 0,
    };

    for (int32_t l = 0; l < 2; l++) {
        const M_ITEM_ROOM_LIGHT *const lights = il->lights[l];
        for (int32_t i = 0; i < il->n_lights[l]; i++) {
            if (lights[i].active) {
                M_StageLight(&lights[i], mesh_pos, &list);
            }
        }
    }
    M_StageDynamicLights(mesh_pos, &list);

    info->tr4.handle = m_StagedPool->count;
    Vector_Add(m_StagedPool, &list);
}

static void M_UploadList(
    const OUTPUT_UNIFORMS *const uniforms, const M_STAGED_LIST *const list)
{
    OUTPUT_UNIFORM_LS_TR4 ls = {};
    for (int32_t i = 0; i < 3; i++) {
        ls.ambient[i] = list->ambient[i] / 255.0f;
    }
    ls.num_lights = list->count;
    for (int32_t i = 0; i < list->count; i++) {
        ls.lights[i].vec[0] = list->lights[i].vec.x;
        ls.lights[i].vec[1] = list->lights[i].vec.y;
        ls.lights[i].vec[2] = list->lights[i].vec.z;
        ls.lights[i].vec[3] = 0.0f;
        ls.lights[i].color[0] = list->lights[i].r;
        ls.lights[i].color[1] = list->lights[i].g;
        ls.lights[i].color[2] = list->lights[i].b;
        ls.lights[i].color[3] = list->lights[i].rad;
    }

    const size_t size = offsetof(OUTPUT_UNIFORM_LS_TR4, lights)
        + list->count * sizeof(ls.lights[0]);
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    TRX_GL_TRACK_SUBDATA(glBufferSubData, GL_UNIFORM_BUFFER, 0, size, &ls);
    TRX_GL_CheckError();
}

static void M_UploadCPULight(
    const OUTPUT_UNIFORMS *const uniforms, const OUTPUT_LIGHT_INFO *const info)
{
    if (info->tr4.handle >= 0) {
        if (m_UploadCache.mode == LS_MODE_FULL
            && m_UploadCache.generation == m_PoolGeneration
            && m_UploadCache.handle == info->tr4.handle) {
            return;
        }
        m_UploadCache.mode = LS_MODE_FULL;
        m_UploadCache.generation = m_PoolGeneration;
        m_UploadCache.handle = info->tr4.handle;
        M_UploadList(uniforms, Vector_Get(m_StagedPool, info->tr4.handle));
        return;
    }

    // Constant light (Output_CalculateStaticLight* paths): treat the
    // multiplier as a pure ambient with no light list. 128 = neutral.
    if (m_UploadCache.mode == LS_MODE_FULL && m_UploadCache.handle == -1
        && m_UploadCache.constant.r == info->tr3_ambient.r
        && m_UploadCache.constant.g == info->tr3_ambient.g
        && m_UploadCache.constant.b == info->tr3_ambient.b) {
        return;
    }
    m_UploadCache.mode = LS_MODE_FULL;
    m_UploadCache.handle = -1;
    m_UploadCache.constant = info->tr3_ambient;

    const M_STAGED_LIST list = {
        .ambient = {
            info->tr3_ambient.r * 128.0f,
            info->tr3_ambient.g * 128.0f,
            info->tr3_ambient.b * 128.0f,
        },
        .count = 0,
    };
    M_UploadList(uniforms, &list);
}

static void M_UploadOwnLight(
    const OUTPUT_UNIFORMS *const uniforms, const OUTPUT_LIGHT_INFO *const info)
{
    if (m_UploadCache.mode == LS_MODE_OWN
        && m_UploadCache.own_ambient.r == info->tr3_ambient.r
        && m_UploadCache.own_ambient.g == info->tr3_ambient.g
        && m_UploadCache.own_ambient.b == info->tr3_ambient.b) {
        return;
    }
    m_UploadCache.mode = LS_MODE_OWN;
    m_UploadCache.own_ambient = info->tr3_ambient;

    const float ambient[4] = {
        info->tr3_ambient.r,
        info->tr3_ambient.g,
        info->tr3_ambient.b,
        0.0f,
    };
    glBindBuffer(GL_UNIFORM_BUFFER, uniforms->ls);
    TRX_GL_TRACK_SUBDATA(
        glBufferSubData, GL_UNIFORM_BUFFER,
        offsetof(OUTPUT_UNIFORM_LS_TR4, ambient), sizeof(ambient), ambient);
}

static void M_CalculateStaticLight(const int16_t adder)
{
    Output_Lights_SetScalarStaticLight(adder);

    float mul = 2.0f - (adder / (float)SHADE_NEUTRAL);
    CLAMP(mul, 0.0f, 1.0f);
    M_SetConstantLight((RGB_F) { mul, mul, mul });
}

static RGB_F M_RGB15ToRGBF(const int16_t rgb15)
{
    const int32_t r8 = (rgb15 & 0x1F) << 3;
    const int32_t g8 = ((rgb15 >> 5) & 0x1F) << 3;
    const int32_t b8 = ((rgb15 >> 10) & 0x1F) << 3;
    return (RGB_F) {
        .r = r8 / 255.0f,
        .g = g8 / 255.0f,
        .b = b8 / 255.0f,
    };
}

static void M_CalculateStaticLightRGB15(const int16_t rgb15)
{
    M_SetConstantLight(M_RGB15ToRGBF(rgb15));
}

static void M_CalculateStaticLightRGB_F(const RGB_F rgb)
{
    M_SetConstantLight(rgb);
}

// Port of the OG ProcessStaticMeshVertices' constant part (output.cpp:263):
// the instance shade is a per-channel multiplier over the mesh prelight.
// The per-vertex dynamic light additions happen in the shader.
static void M_CalculateStaticMeshLight(
    const XYZ_32 pos, const SHADE shade, const ROOM *const room)
{
    M_SetConstantLight(M_RGB15ToRGBF(shade.value_1 & 0x7FFF));
    M_SetScalarFallback((RGB_888) { 128, 128, 128 });
}

// Port of the OG TriggerDynamic (effect2.cpp:475).
static void M_AddDynamicLightRGB(
    const XYZ_32 pos, const int32_t falloff, const RGB_888 color)
{
    int32_t safe_falloff = falloff;
    CLAMP(safe_falloff, 0, OUTPUT_DYNAMIC_FALLOFF_MAX);

    RGB_888 c = color;
    if (safe_falloff < 8) {
        c.r = (c.r * safe_falloff) >> 3;
        c.g = (c.g * safe_falloff) >> 3;
        c.b = (c.b * safe_falloff) >> 3;
    }

    const OUTPUT_DYNAMIC_LIGHT light = {
        .light = {
            .pos = pos,
            .color = c,
            .layout = LIGHT_LAYOUT_LEGACY,
            .type = LIGHT_TYPE_POINT,
            .u.legacy =
                {
                    .falloff.value_1 = safe_falloff
                        << OUTPUT_DYNAMIC_FALLOFF_SHIFT,
                },
        },
        .kind = OUTPUT_DYNAMIC_LIGHT_RGB,
    };
    Vector_Add(Output_GetDynamicLights(), &light);
}

static void M_Init(void)
{
    if (m_StagedPool == nullptr) {
        m_StagedPool = Vector_Create(sizeof(M_STAGED_LIST));
    }
}

static void M_Shutdown(void)
{
    if (m_StagedPool != nullptr) {
        Vector_Free(m_StagedPool);
        m_StagedPool = nullptr;
    }
    M_FreeRoomBake();
}

static void M_ObserveLevelLoad(void)
{
    memset(m_ItemLights, 0, sizeof(m_ItemLights));
    memset(&m_ScratchLight, 0, sizeof(m_ScratchLight));
    memset(m_LooseLightKeys, 0, sizeof(m_LooseLightKeys));
    memset(m_LooseLights, 0, sizeof(m_LooseLights));
    m_CurrentItemLight = nullptr;
    m_CurrentHasList = false;
    if (m_StagedPool != nullptr) {
        Vector_Clear(m_StagedPool);
    }
    if (g_TRVersion == 4) {
        M_BakeRoomLights();
    } else {
        M_FreeRoomBake();
    }
}

static void M_BeginScene(void)
{
    if (m_StagedPool != nullptr) {
        Vector_Clear(m_StagedPool);
    }
    m_PoolGeneration++;
}

void Output_SetInventoryLightingMode(const bool enabled)
{
    m_InventoryMode = enabled;
}

const LIGHTING_MODEL g_LightingModelTR4 = {
    .init = M_Init,
    .shutdown = M_Shutdown,
    .observe_level_load = M_ObserveLevelLoad,
    .calculate_light = M_CalculateLight,
    .calculate_object_lighting_at = M_CalculateObjectLightingAt,
    .calculate_static_light = M_CalculateStaticLight,
    .calculate_static_light_rgb15 = M_CalculateStaticLightRGB15,
    .calculate_static_light_rgb_f = M_CalculateStaticLightRGB_F,
    .calculate_static_mesh_light = M_CalculateStaticMeshLight,
    .add_dynamic_light = Output_Lights_TR3_AddDynamicLight,
    .add_dynamic_light_rgb = M_AddDynamicLightRGB,
    .begin_scene = M_BeginScene,
    .prepare_scene = Output_FogBulbs_PrepareScene,
    .animate = Output_FogBulbs_Animate,
    .fill_instance_light = M_FillInstanceLight,
    .upload_cpu_light = M_UploadCPULight,
    .upload_own_light = M_UploadOwnLight,
    .shader_variant = 2,
};
