#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/vector.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/traps/common.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

#define M_LARA_BREAK_TIME (LOGIC_FPS * 2) // = 60
#define M_HEAVY_BREAK_TIME 2

typedef struct {
    bool has_animation_data;
    bool requires_heavy_trigger;
    bool heavy_triggered;
    int32_t origin;
    int32_t break_timer;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    SHOULD(JSON_READ(io, "break_timer", &p->break_timer));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "break_timer", p->break_timer);
}

static int32_t M_GetOrigin(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->origin;
}

static void M_CalculateOrigin(ITEM *const item)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM *const anim = Object_GetAnim(obj, 0);
    const ANIM_FRAME *const frame = &anim->frame_ptr[0];
    M_PRIV *const p = item->priv;
    p->origin = ROUND_TO_CLICK_SIGNED(frame->offset.y);
}

static void M_Initialise(const int16_t item_num)
{
    Trap_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->has_animation_data = Anim_HasChange(Item_GetAnim(item), TRAP_ACTIVATE);

    if (p->has_animation_data) {
        M_CalculateOrigin(item);
    } else {
        item->mesh_bits = 1;
        TRX_VALUE value;
        if (ObjectProperty_GetItemValue(item, "requires_heavy_trigger", &value)
            && value.as_bool) {
            p->requires_heavy_trigger = true;
            p->break_timer = M_HEAVY_BREAK_TIME;
        }
    }

    Walkable_AllocateNodes(item, 1);
}

static XYZ_32 M_GetStackPos(const ITEM *const item)
{
    const int32_t origin = M_GetOrigin(item);
    return (XYZ_32) {
        .x = item->pos.x,
        .y = item->pos.y + origin,
        .z = item->pos.z,
    };
}

static void M_LockStack(const ITEM *const item)
{
    // Once the floor begins to break, any walkables on the stack cannot be
    // moved by Lara.
    const XYZ_32 drop_pos = M_GetStackPos(item);
    MovableBlock_LockStack(drop_pos, item->room_num);
}

static void M_DropStack(const ITEM *const item)
{
    // Once the floor has broken, drop any stacked walkables.
    const XYZ_32 drop_pos = M_GetStackPos(item);
    MovableBlock_DropStack(drop_pos, item->room_num);
}

static int32_t M_GetFloorHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    const int32_t origin = M_GetOrigin(item);
    if (pos.y <= item->pos.y + origin
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y + origin;
    }
    return height;
}

static int32_t M_GetCeilingHeight(
    const ITEM *const item, const XYZ_32 pos, const int32_t height)
{
    const int32_t origin = M_GetOrigin(item);
    if (pos.y > item->pos.y + origin
        && (item->current_anim_state == TRAP_SET
            || item->current_anim_state == TRAP_ACTIVATE)) {
        return item->pos.y + origin + STEP_L;
    }
    return height;
}

static void M_AddWalkable(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    Walkable_Add(item_num, item->pos);
}

static bool M_Trigger(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    M_PRIV *const p = item->priv;
    p->heavy_triggered = trigger->kind == ITEM_TRIGGER_HEAVY;
    return true;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->has_animation_data || p->requires_heavy_trigger
        || p->break_timer != 0) {
        return;
    }

    if (lara_item->pos.y != item->pos.y
        || ROUND_TO_SECTOR(lara_item->pos.x) != ROUND_TO_SECTOR(item->pos.x)
        || ROUND_TO_SECTOR(lara_item->pos.z) != ROUND_TO_SECTOR(item->pos.z)) {
        return;
    }

    Sound_Effect(SFX_ROCK_FALL_CRUMBLE, &item->pos, SPM_NORMAL);
    Item_AddSimulated(item_num);
    item->trigger.mask = TRIGGER_MASK_ALL;
    p->break_timer = M_LARA_BREAK_TIME;
    M_LockStack(item);
}

static void M_ControlAnimated(ITEM *const item)
{
    const int32_t item_num = Item_GetIndex(item);
    const int32_t origin = M_GetOrigin(item);

    switch (item->current_anim_state) {
    case TRAP_SET:
        const ITEM *const lara_item = Lara_GetItem();
        M_PRIV *const p = item->priv;
        if (!p->heavy_triggered && lara_item->pos.y != item->pos.y + origin) {
            Item_RemoveSimulated(item_num);
            return;
        }
        if (item->goal_anim_state != TRAP_ACTIVATE) {
            item->goal_anim_state = TRAP_ACTIVATE;
            M_LockStack(item);
        }
        p->heavy_triggered = false;
        break;

    case TRAP_ACTIVATE:
        item->goal_anim_state = TRAP_WORKING;
        break;

    case TRAP_WORKING:
        if (item->goal_anim_state != TRAP_FINISHED) {
            if (!item->gravity) {
                M_DropStack(item);
            }
            item->gravity = true;
        }
        break;

    default:
        break;
    }

    Item_Animate(item);
    if (item->is_finished) {
        if (!Item_IsTriggerActive(item)) {
            Trap_Reset(item);
        }
        return;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    Item_UpdateRoom(item_num, room_num);

    item->floor = Room_GetHeight(sector, item->pos);

    if (item->current_anim_state == TRAP_WORKING
        && item->pos.y >= item->floor) {
        item->goal_anim_state = TRAP_FINISHED;
        item->pos.y = item->floor;
        item->fall_speed = 0;
        item->gravity = false;
    }
}

static void M_ControlSimulated(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (p->break_timer <= 1) {
        M_DropStack(item);
        item->mesh_bits = 0xFFFFFFFE;
        item->current_anim_state = TRAP_FINISHED;

        const int32_t item_num = Item_GetIndex(item);
        Item_Shatter(item_num, -1, 2465);
        Item_Destroy(item_num);
    } else {
        item->rot.x = (Random_GetControl() & 0x3FF) - 0x200;
        item->rot.z = (Random_GetControl() & 0x3FF) - 0x200;
        p->break_timer--;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->has_animation_data) {
        M_ControlAnimated(item);
    } else {
        M_ControlSimulated(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;
    obj->trigger_func = M_Trigger;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->floor_height_func = M_GetFloorHeight;
    obj->ceiling_height_func = M_GetCeilingHeight;
    obj->add_walkable_func = M_AddWalkable;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, requires_heavy_trigger, false,
            "Whether the block can only be activated from heavy triggers."));
}

REGISTER_OBJECT(O_FALLING_BLOCK_1, M_Setup)
REGISTER_OBJECT(O_FALLING_BLOCK_2, M_Setup)
REGISTER_OBJECT(O_FALLING_BLOCK_3, M_Setup)
