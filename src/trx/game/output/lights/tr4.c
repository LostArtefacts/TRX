// TR4 lighting family: port of the OG per-item colored light pipeline
// (specific/lighting.cpp). Room lights are baked per level into a compact
// table (ProcessRoomData port); each item keeps current/previous light lists
// with 8-frame fades (CreateLightList/FadeLightList/CalcAmbientLight ports);
// each staged mesh instance resolves the lists into a flat light list
// (SetupLight port) consumed by the vertex shader.

#include <trx/core/colors.h>
#include <trx/core/math/geom.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/camera/vars.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/const.h>
#include <trx/game/items/manager.h>
#include <trx/game/level/settings.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/output/lights/fog_bulbs.h>
#include <trx/game/output/lights/priv.h>
#include <trx/game/output/water.h>
#include <trx/game/random.h>
#include <trx/version.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define M_MAX_ITEM_LIGHTS 21 // the OG ITEM_LIGHT current/prev list capacity
// Enough for a cutscene's cast, which is what asks to be lit without living
// in the item pool.
#define M_MAX_LOOSE_LIGHTS 16
#define M_FADE_FRAMES 8
// The TR4 object slot of GAME_PIECE1. Lara, the vehicles and the creatures
// come before it; the OG lights every object from it on as a prop.
#define M_FIRST_PROP_SLOT 108
// How many of the light lists put in batches last a new list is matched
// against.
#define M_BATCH_RUNS 32

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

// Matches TR4_LIGHT_* in lights_tr4.glsl.
typedef enum {
    M_STAGED_POINT = 0,
    M_STAGED_SPOT = 1,
    M_STAGED_SUN = 2,
} M_STAGED_TYPE;

// Fully resolved per-mesh-instance light list handed to the GPU. The lights
// stay in world space, so that the meshes of one item share one list in a
// draw; the shader carries them into the object space of each mesh.
typedef struct {
    float ambient[3]; // 0..255 scale (128 = OG neutral)
    XYZ_32 item_pos;
    int32_t count;
    struct {
        M_STAGED_TYPE type;
        XYZ_F pos;
        XYZ_F dir; // where a spot points or a sun shines
        float r, g, b; // 0..1
        float range;
        float att1;
        float cone; // cosine of a spot's cone edge
    } lights[OUTPUT_TR4_MAX_STAGED_LIGHTS];
} M_STAGED_LIST;

static M_ROOM_BAKE *m_RoomBake = nullptr;
static int32_t m_RoomBakeCount = 0;

static M_ITEM_LIGHT m_ItemLights[MAX_ITEMS] = {};
static M_ITEM_LIGHT m_ScratchLight = {}; // the OG StaticMeshLightItem
static const ITEM *m_LooseLightKeys[M_MAX_LOOSE_LIGHTS] = {};
static M_ITEM_LIGHT m_LooseLights[M_MAX_LOOSE_LIGHTS] = {};
static M_ITEM_LIGHT *m_CurrentItemLight = nullptr;
static bool m_CurrentIsProp = false; // the OG SetupLight_thing
static bool m_CurrentHasList = false;
static bool m_InventoryMode = false;

static VECTOR *m_StagedPool = nullptr; // of M_STAGED_LIST

// The lists put in batch light lists last, so that the meshes of one item
// share the list of the item. A run counts only while the batch's list still
// holds the same lights where it says.
static struct {
    int32_t first;
    int32_t count;
    uint32_t hash;
} m_BatchRuns[M_BATCH_RUNS] = {};
static int32_t m_BatchRunNext = 0;

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

static bool M_HasRoom(const M_STAGED_LIST *const list)
{
    return list->count < OUTPUT_TR4_MAX_STAGED_LIGHTS;
}

// Port of the retail SetupLight. Every light sits where it is relative to
// the item, which the shader carries into the object space of the mesh being
// drawn; the shadow lights only darken the ambient.
static void M_StageLight(
    const M_ITEM_ROOM_LIGHT *const light, M_STAGED_LIST *const list)
{
    const XYZ_F pos = {
        (float)light->ipos.x,
        (float)light->ipos.y,
        (float)light->ipos.z,
    };
    const XYZ_F dir = { light->nx, light->ny, light->nz };

    switch (light->type) {
    case LIGHT_TYPE_SUN: {
        if (!M_HasRoom(list)) {
            break;
        }
        const float mul = m_InventoryMode ? 2.0f : 1.0f;
        list->lights[list->count++] = (typeof(list->lights[0])) {
            .type = M_STAGED_SUN,
            .dir = dir,
            .r = light->r * mul,
            .g = light->g * mul,
            .b = light->b * mul,
        };
        break;
    }

    case LIGHT_TYPE_POINT: {
        if (m_CurrentIsProp) {
            // A prop takes a point light it is inside into its ambient as
            // well, by how far into the light it stands.
            const float rad =
                (light->outer - sqrtf((float)light->range)) / light->outer;
            if (rad < 1.0f) {
                const float c[3] = { light->r, light->g, light->b };
                for (int32_t i = 0; i < 3; i++) {
                    list->ambient[i] += rad * c[i] * 255.0f;
                    CLAMP(list->ambient[i], 0.0f, 255.0f);
                }
            }
        }
        if (!M_HasRoom(list)) {
            break;
        }
        list->lights[list->count++] = (typeof(list->lights[0])) {
            .type = M_STAGED_POINT,
            .pos = pos,
            .r = light->r,
            .g = light->g,
            .b = light->b,
            .range = light->outer,
            .att1 = 2.0f,
        };
        break;
    }

    case LIGHT_TYPE_SPOT:
        if (!M_HasRoom(list)) {
            break;
        }
        list->lights[list->count++] = (typeof(list->lights[0])) {
            .type = M_STAGED_SPOT,
            .pos = pos,
            .dir = dir,
            .r = light->r,
            .g = light->g,
            .b = light->b,
            .range = light->cutoff,
            .att1 = m_CurrentIsProp ? 2.0f : 1.0f,
            .cone = light->outer,
        };
        break;

    case LIGHT_TYPE_SHADOW: {
        const float val = (float)(int32_t)sqrtf((float)light->range);
        int32_t val2 = light->shadow >> 3;
        if (val >= light->inner) {
            val2 = (int32_t)((val - light->outer)
                             / ((light->outer - light->inner) / (float)-val2));
        }
        for (int32_t i = 0; i < 3; i++) {
            list->ambient[i] -= (float)val2;
            CLAMP(list->ambient[i], 0.0f, 255.0f);
        }
        break;
    }

    default:
        break;
    }
}

// Port of the retail dynamic lights in InitItemDynamicLighting.
static void M_StageDynamicLights(M_STAGED_LIST *const list)
{
    VECTOR *const dynamic_lights = Output_GetDynamicLights();
    for (int32_t i = 0; i < dynamic_lights->count; i++) {
        if (!M_HasRoom(list)) {
            break;
        }
        const OUTPUT_DYNAMIC_LIGHT *const entry = Vector_Get(dynamic_lights, i);
        const LIGHT *const light = &entry->light;
        const int32_t raw_falloff = light->u.legacy.falloff.value_1;
        const float range = (raw_falloff >> 1) + (raw_falloff >> 3);
        if (range <= 0.0f) {
            continue;
        }

        list->lights[list->count++] = (typeof(list->lights[0])) {
            .type = M_STAGED_POINT,
            .pos = {
                (float)light->pos.x,
                (float)light->pos.y,
                (float)light->pos.z,
            },
            .r = light->color.r / 255.0f,
            .g = light->color.g / 255.0f,
            .b = light->color.b / 255.0f,
            .range = range,
            .att1 = 1.0f,
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
    m_CurrentIsProp = item != nullptr
        && Catalog_IDToSlot(CATALOG_OBJECTS, item->object_id, -1)
            >= M_FIRST_PROP_SLOT;
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
    m_CurrentIsProp = false;
    m_CurrentHasList = true;

    M_SetScalarFallback(il->ambient);
}

static void M_FillInstanceLight(OUTPUT_LIGHT_INFO *const info)
{
    info->tr4.handle = -1;
    if (!m_CurrentHasList || m_CurrentItemLight == nullptr) {
        return;
    }

    const M_ITEM_LIGHT *const il = m_CurrentItemLight;

    // Selects a Lara mesh's ambient light from the room it sits in. The
    // original lights all of Lara from one room.
    const RGB_888 *const mesh_ambient = Output_Water_GetLaraMeshAmbient();
    const RGB_888 ambient =
        mesh_ambient != nullptr ? *mesh_ambient : il->ambient;
    M_STAGED_LIST list = {
        .ambient = { ambient.r, ambient.g, ambient.b },
        .item_pos = il->item_pos,
        .count = 0,
    };

    for (int32_t l = 0; l < 2; l++) {
        const M_ITEM_ROOM_LIGHT *const lights = il->lights[l];
        for (int32_t i = 0; i < il->n_lights[l]; i++) {
            if (lights[i].active) {
                M_StageLight(&lights[i], &list);
            }
        }
    }
    M_StageDynamicLights(&list);

    info->tr4.handle = m_StagedPool->count;
    Vector_Add(m_StagedPool, &list);
}

static uint32_t M_HashInstanceLights(
    const OUTPUT_MESH_LIGHT *const lights, const int32_t count)
{
    const uint8_t *const bytes = (const uint8_t *)lights;
    uint32_t hash = 2166136261u;
    for (size_t i = 0; i < count * sizeof(OUTPUT_MESH_LIGHT); i++) {
        hash = (hash ^ bytes[i]) * 16777619u;
    }
    return hash;
}

// Returns where the lights start in the draw's list, adding them unless the
// list already holds them.
static int32_t M_PlaceInstanceLights(
    OUTPUT_MESH_LIGHT_LIST *const lights, const OUTPUT_MESH_LIGHT *const src,
    const int32_t count)
{
    const uint32_t hash = M_HashInstanceLights(src, count);
    for (int32_t i = 0; i < M_BATCH_RUNS; i++) {
        const typeof(m_BatchRuns[0]) *const run = &m_BatchRuns[i];
        if (run->count == count && run->hash == hash
            && run->first + count <= lights->count
            && memcmp(
                   &lights->data[run->first], src,
                   count * sizeof(OUTPUT_MESH_LIGHT))
                == 0) {
            return run->first;
        }
    }

    ASSERT(lights->count + count <= lights->capacity);
    const int32_t first = lights->count;
    memcpy(&lights->data[first], src, count * sizeof(OUTPUT_MESH_LIGHT));
    lights->count += count;
    m_BatchRuns[m_BatchRunNext] = (typeof(m_BatchRuns[0])) {
        .first = first,
        .count = count,
        .hash = hash,
    };
    m_BatchRunNext = (m_BatchRunNext + 1) % M_BATCH_RUNS;
    return first;
}

// Fills the slot with the light of the instance. Row 0 holds the ambient,
// row 1 the item position, and row 2 where the lights start in the draw's
// list and how many there are.
static void M_FillSlotLight(
    const OUTPUT_LIGHT_INFO *const info, const bool own, float out[7][4],
    OUTPUT_MESH_LIGHT_LIST *const lights)
{
    if (own) {
        out[0][0] = info->tr3_ambient.r;
        out[0][1] = info->tr3_ambient.g;
        out[0][2] = info->tr3_ambient.b;
        return;
    }

    // Constant light (Output_CalculateStaticLight* paths) is a pure ambient
    // with no light list. 128 = neutral.
    if (info->tr4.handle < 0) {
        out[0][0] = info->tr3_ambient.r * 128.0f / 255.0f;
        out[0][1] = info->tr3_ambient.g * 128.0f / 255.0f;
        out[0][2] = info->tr3_ambient.b * 128.0f / 255.0f;
        return;
    }

    const M_STAGED_LIST *const list =
        Vector_Get(m_StagedPool, info->tr4.handle);
    for (int32_t i = 0; i < 3; i++) {
        out[0][i] = list->ambient[i] / 255.0f;
    }
    out[1][0] = (float)list->item_pos.x;
    out[1][1] = (float)list->item_pos.y;
    out[1][2] = (float)list->item_pos.z;
    if (list->count == 0) {
        return;
    }

    OUTPUT_MESH_LIGHT src[OUTPUT_TR4_MAX_STAGED_LIGHTS] = {};
    for (int32_t i = 0; i < list->count; i++) {
        const typeof(list->lights[0]) *const light = &list->lights[i];
        src[i] = (OUTPUT_MESH_LIGHT) {
            .pos = { light->pos.x, light->pos.y, light->pos.z,
                     (float)light->type },
            .color = { light->r, light->g, light->b, light->range },
            .dir = { light->dir.x, light->dir.y, light->dir.z, light->att1 },
            .cone = { light->cone },
        };
    }
    out[2][0] = (float)M_PlaceInstanceLights(lights, src, list->count);
    out[2][1] = (float)list->count;
}

static int32_t M_GetSlotLightCount(const OUTPUT_LIGHT_INFO *const info)
{
    if (info->tr4.handle < 0) {
        return 0;
    }
    const M_STAGED_LIST *const list =
        Vector_Get(m_StagedPool, info->tr4.handle);
    return list->count;
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

static void M_ObserveRoomFlip(const ROOM *const room)
{
    if (m_RoomBake == nullptr || room->flip_status != RFS_UNFLIPPED
        || room->flipped_room == NO_ROOM) {
        return;
    }
    const int16_t room_1 = Room_GetIndex(room);
    const int16_t room_2 = room->flipped_room;
    SWAP(m_RoomBake[room_1], m_RoomBake[room_2]);
}

static void M_BeginScene(void)
{
    if (m_StagedPool != nullptr) {
        Vector_Clear(m_StagedPool);
    }
}

void Output_SetInventoryLightingMode(const bool enabled)
{
    m_InventoryMode = enabled;
}

const LIGHTING_MODEL g_LightingModelTR4 = {
    .init = M_Init,
    .shutdown = M_Shutdown,
    .observe_level_load = M_ObserveLevelLoad,
    .observe_room_flip = M_ObserveRoomFlip,
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
    .fill_slot_light = M_FillSlotLight,
    .get_slot_light_count = M_GetSlotLightCount,
    .shader_variant = 2,
};
