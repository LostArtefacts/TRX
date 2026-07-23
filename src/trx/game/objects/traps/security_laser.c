#include <trx/core/utils.h>
#include <trx/game/lara.h>
#include <trx/game/los.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/pathing.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>

#define M_DEFAULT_DAMAGE 10

static uint8_t m_LaserShades[32] = {};
static const int16_t m_DefaultBeamCount = 1;

static int32_t M_GetDamage(const ITEM *const item)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DEFAULT_DAMAGE;
}

static bool M_IsTargetable(const ITEM *const item)
{
    return false;
}

static void M_LaserSplitterToggle(ITEM *const item)
{
    int16_t room_num = item->room_num;
    SECTOR *sector = Room_GetSector(item->pos, &room_num);

    const BOX_INFO *const box = Box_GetBox(sector->box);
    if (box == nullptr || (box->overlap_index & BOX_BLOCKED_SEARCH) == 0) {
        return;
    }

    const bool is_active = Item_IsTriggerActive(item);

    if (is_active == ((box->overlap_index & BOX_BLOCKED) == BOX_BLOCKED)) {
        return;
    }

    XZ_32 step;
    switch (item->rot.y) {
    case 0:
        step = (XZ_32) { 0, -WALL_L };
        break;
    case DEG_90:
        step = (XZ_32) { -WALL_L, 0 };
        break;
    case -DEG_180:
        step = (XZ_32) { 0, WALL_L };
        break;
    default:
        step = (XZ_32) { WALL_L, 0 };
        break;
    }

    int32_t x = item->pos.x;
    int32_t z = item->pos.z;
    BOX_INFO *next_box;
    while (sector->box != NO_BOX) {
        next_box = Box_GetBox(sector->box);
        if (next_box == nullptr
            || (next_box->overlap_index & BOX_BLOCKED_SEARCH) == 0) {
            break;
        }

        TOGGLE_BIT(next_box->overlap_index, BOX_BLOCKED, is_active);
        x += step.x;
        z += step.z;
        sector = Room_GetSector((XYZ_32) { x, item->pos.y, z }, &room_num);
    }
}

static void M_UpdateLaserShades(void)
{
    for (int32_t shade_idx = 0; shade_idx < 32; shade_idx++) {
        uint8_t shade = m_LaserShades[shade_idx];
        int32_t random_value = Random_GetDraw();

        if (random_value < 1024) {
            random_value = (random_value & 0xF) + 16;
        } else if (random_value < 4096) {
            random_value &= 7;
        } else if ((random_value & 0x70) == 0) {
            random_value &= 3;
        } else {
            random_value = 0;
        }

        if (random_value != 0) {
            shade += (uint8_t)random_value;

            if (shade > 127) {
                shade = 127;
            }
        } else if (shade > 16) {
            shade -= shade >> 3;
        } else {
            shade = 16;
        }

        m_LaserShades[shade_idx] = shade;
    }
}

static RGB_888 M_GetLaserColor(const OBJECT_ID object_id)
{
    switch (object_id) {
    case O_SECURITY_LASER_ALARM:
        return (RGB_888) { 0, UINT8_MAX, 0 };
    case O_SECURITY_LASER_DEADLY:
        return (RGB_888) { UINT8_MAX, UINT8_MAX, 0 };
    default:
        return (RGB_888) { UINT8_MAX, UINT8_MAX >> 2, 0 };
    }
}

static XZ_32 M_GetLaserDirection(const ITEM *const item)
{
    const int32_t edge_offset = WALL_L / 2 - 1;
    switch (item->rot.y) {
    case 0:
        return (XZ_32) { 0, edge_offset };
    case DEG_90:
        return (XZ_32) { edge_offset, 0 };
    case -DEG_180:
        return (XZ_32) { 0, -edge_offset };
    default:
        return (XZ_32) { -edge_offset, 0 };
    }
}

static bool M_GetLaserSegment(
    const ITEM *const item, const XZ_32 dir, const int32_t y,
    GAME_VECTOR *const s, GAME_VECTOR *const t)
{
    s->x = item->pos.x + dir.x;
    s->y = item->pos.y + y;
    s->z = item->pos.z + dir.z;
    s->room_num = item->room_num;

    t->x = item->pos.x - (dir.x << 5);
    t->y = item->pos.y + y;
    t->z = item->pos.z - (dir.z << 5);

    LOS_Check(s, t, true);
    return LOS_CheckItemIntersectSegment(s, t, Lara_GetItem());
}

static void M_DrawLaserBeam(
    const GAME_VECTOR *const src, const GAME_VECTOR *const dest,
    const RGB_888 color)
{
    const XYZ_32 beam_delta = {
        .x = dest->x - src->x,
        .y = dest->y - src->y,
        .z = dest->z - src->z,
    };

    int32_t segment_count =
        XYZ_32_GetDistance(
            (XYZ_32) { src->x, 0, src->z }, (XYZ_32) { dest->x, 0, dest->z })
        >> 9;
    CLAMP(segment_count, 8, 32);

    XYZ_32 segment_start = src->pos;
    for (int32_t segment_idx = 0; segment_idx < segment_count; segment_idx++) {
        const float segment_end_ratio =
            (float)(segment_idx + 1) / segment_count;
        const XYZ_32 segment_end = {
            .x = src->x + (int32_t)(beam_delta.x * segment_end_ratio),
            .y = src->y + (int32_t)(beam_delta.y * segment_end_ratio),
            .z = src->z + (int32_t)(beam_delta.z * segment_end_ratio),
        };

        RGBA_8888 from_color = COLOR_RGBA_8888_BLACK;
        RGBA_8888 to_color = COLOR_RGBA_8888_BLACK;

        if (segment_idx > 0) {
            const uint8_t shade = m_LaserShades[segment_idx];
            from_color = (RGBA_8888) {
                .r = (uint8_t)((shade * color.r) / UINT8_MAX),
                .g = (uint8_t)((shade * color.g) / UINT8_MAX),
                .b = (uint8_t)((shade * color.b) / UINT8_MAX),
                .a = 0xFF,
            };
        }

        if (segment_idx + 1 < segment_count) {
            const uint8_t shade = m_LaserShades[segment_idx + 1];
            to_color = (RGBA_8888) {
                .r = (uint8_t)((shade * color.r) / UINT8_MAX),
                .g = (uint8_t)((shade * color.g) / UINT8_MAX),
                .b = (uint8_t)((shade * color.b) / UINT8_MAX),
                .a = 0xFF,
            };
        }

        OutputSource_PolyFX_StageLineSegment(
            segment_start, from_color, segment_end, to_color, 4.0f,
            DRAW_BLEND_ADD);
        segment_start = segment_end;
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_LaserSplitterToggle(item);
}

static void M_DamageLara(const ITEM *const item, const int32_t beam_y)
{
    if (item->object_id == O_SECURITY_LASER_ALARM) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    if (item->object_id == O_SECURITY_LASER_KILLER) {
        Lara_TakeDamage(lara_item->hit_points, false);
    } else {
        Lara_TakeDamage(M_GetDamage(item), false);
    }

    Spawn_BloodBath(
        lara_item->pos.x, item->pos.y + beam_y, lara_item->pos.z,
        (Random_GetDraw() & 0x7F) + 128, Random_GetDraw() << 1,
        lara_item->room_num, 1);
}

static void M_ActivateTriggers(const ITEM *const item)
{
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const target_item = Item_Get(i);
        if ((target_item->object_id != O_STROBE_LIGHT
             && target_item->object_id != O_SENTRY_GUN)
            || !Item_IsTriggerActive(target_item)) {
            continue;
        }

        const OBJECT *const target_obj = Object_Get(target_item->object_id);
        if (target_obj->event_func != nullptr) {
            target_obj->event_func(target_item, OBJECT_EVENT_ALERT, nullptr);
        }
    }

    Room_TestTriggers(item);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_LaserSplitterToggle(item);

    if (!Item_IsTriggerActiveRO(item)) {
        return;
    }

    M_UpdateLaserShades();

    const XZ_32 direction = M_GetLaserDirection(item);

    int32_t beam_y = 0;
    bool tripped = false;
    for (int32_t beam_idx = 0; beam_idx < item->hit_points; beam_idx++) {
        GAME_VECTOR start;
        GAME_VECTOR target;

        if (M_GetLaserSegment(item, direction, beam_y, &start, &target)) {
            tripped = true;
            M_DamageLara(item, beam_y);
        }
        beam_y -= 256;
    }

    if (tripped) {
        M_ActivateTriggers(item);
    }
}

static bool M_Trigger(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    if (trigger->kind != ITEM_TRIGGER_ANTI) {
        item->hit_points = (int16_t)trigger->timer & 7;
        if (item->hit_points == 0) {
            item->hit_points = 1;
        }
        item->max_hit_points = item->hit_points;
        item->timer = 0;
    }

    return true;
}

static bool M_DrawLaser(const ITEM *const item)
{
    const RGB_888 color = M_GetLaserColor(item->object_id);

    if (!Item_IsTriggerActiveRO(item)) {
        return false;
    }

    const XZ_32 direction = M_GetLaserDirection(item);
    int32_t beam_y = 0;
    for (int32_t beam_idx = 0; beam_idx < item->hit_points; beam_idx++) {
        GAME_VECTOR start;
        GAME_VECTOR target;
        M_GetLaserSegment(item, direction, beam_y, &start, &target);
        M_DrawLaserBeam(&start, &target, color);
        beam_y -= 256;
    }
    return true;
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_DrawLaser;
    obj->is_targetable_func = M_IsTargetable;
    obj->trigger_func = M_Trigger;

    obj->save_hitpoints = true;
    obj->save_flags = true;
}

static void M_SetupAlarm(OBJECT *const obj)
{
    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", m_DefaultBeamCount, "Maximum hit points."));
}

static void M_SetupDeadly(OBJECT *const obj)
{
    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "damage", M_DEFAULT_DAMAGE,
            "Damage dealt when Lara crosses the security laser."),
        OBJECT_PROPERTY_INT(
            "max_hit_points", m_DefaultBeamCount, "Maximum hit points."));
}

static void M_SetupKiller(OBJECT *const obj)
{
    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", m_DefaultBeamCount, "Maximum hit points."));
}

REGISTER_OBJECT(O_SECURITY_LASER_ALARM, M_SetupAlarm)
REGISTER_OBJECT(O_SECURITY_LASER_DEADLY, M_SetupDeadly)
REGISTER_OBJECT(O_SECURITY_LASER_KILLER, M_SetupKiller)
