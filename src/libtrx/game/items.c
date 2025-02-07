#include "game/items.h"

#include "game/const.h"
#include "game/game_buf.h"
#include "game/item_actions.h"
#include "game/lara/common.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/rooms.h"
#include "game/sound/common.h"
#include "utils.h"

static int32_t m_LevelItemCount = 0;
static int16_t m_MaxUsedItemCount = 0;
static ITEM *m_Items = nullptr;
static int16_t m_NextItemActive = NO_ITEM;
static int16_t m_PrevItemActive = NO_ITEM;
static int16_t m_NextItemFree = NO_ITEM;

void Item_InitialiseItems(const int32_t num_items)
{
    m_Items = GameBuf_Alloc(sizeof(ITEM) * MAX_ITEMS, GBUF_ITEMS);
    m_LevelItemCount = num_items;
    m_MaxUsedItemCount = num_items;
    m_NextItemFree = num_items;
    m_NextItemActive = NO_ITEM;
    m_PrevItemActive = NO_ITEM;

    for (int32_t i = m_NextItemFree; i < MAX_ITEMS - 1; i++) {
        ITEM *const item = &m_Items[i];
        item->active = 0;
        item->next_item = i + 1;
    }
    m_Items[MAX_ITEMS - 1].next_item = NO_ITEM;
}

ITEM *Item_Get(const int16_t item_num)
{
    if (item_num == NO_ITEM) {
        return nullptr;
    }
    return &m_Items[item_num];
}

int32_t Item_GetLevelCount(void)
{
    return m_LevelItemCount;
}

int32_t Item_GetTotalCount(void)
{
    return m_MaxUsedItemCount;
}

int16_t Item_GetIndex(const ITEM *const item)
{
    return item - Item_Get(0);
}

int16_t Item_GetNextActive(void)
{
    return m_NextItemActive;
}

int16_t Item_GetPrevActive(void)
{
    return m_PrevItemActive;
}

void Item_SetPrevActive(const int16_t item_num)
{
    m_PrevItemActive = item_num;
}

int16_t Item_Create(void)
{
    const int16_t item_num = m_NextItemFree;
    if (item_num != NO_ITEM) {
        m_Items[item_num].flags = 0;
        m_NextItemFree = m_Items[item_num].next_item;
    }
    m_MaxUsedItemCount = MAX(m_MaxUsedItemCount, item_num + 1);
    return item_num;
}

int16_t Item_CreateLevelItem(void)
{
    const int16_t item_num = Item_Create();
    if (item_num != NO_ITEM) {
        m_LevelItemCount++;
    }
    return item_num;
}

void Item_Kill(const int16_t item_num)
{
    Item_RemoveActive(item_num);
    Item_RemoveDrawn(item_num);

    ITEM *const item = &m_Items[item_num];
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (item == lara->target) {
        lara->target = nullptr;
    }

#if TR_VERSION == 1
    item->hit_points = -1;
    item->flags |= IF_KILLED;
#else
    if (item_num < m_LevelItemCount) {
        item->flags |= IF_KILLED;
    }
#endif
    if (item_num >= m_LevelItemCount) {
        item->next_item = m_NextItemFree;
        m_NextItemFree = item_num;
    }

    while (m_MaxUsedItemCount > 0
           && m_Items[m_MaxUsedItemCount - 1].flags & IF_KILLED) {
        m_MaxUsedItemCount--;
    }
}

void Item_RemoveActive(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    if (!item->active) {
        return;
    }

    item->active = 0;

    int16_t link_num = m_NextItemActive;
    if (link_num == item_num) {
        m_NextItemActive = item->next_active;
        return;
    }

    while (link_num != NO_ITEM) {
        if (m_Items[link_num].next_active == item_num) {
            m_Items[link_num].next_active = item->next_active;
            return;
        }
        link_num = m_Items[link_num].next_active;
    }
}

void Item_RemoveDrawn(const int16_t item_num)
{
    const ITEM *const item = &m_Items[item_num];
    if (item->room_num == NO_ROOM) {
        return;
    }

    ROOM *const room = Room_Get(item->room_num);
    int16_t link_num = room->item_num;
    if (link_num == item_num) {
        room->item_num = item->next_item;
        return;
    }

    while (link_num != NO_ITEM) {
        if (m_Items[link_num].next_item == item_num) {
            m_Items[link_num].next_item = item->next_item;
            return;
        }
        link_num = m_Items[link_num].next_item;
    }
}

void Item_AddActive(const int16_t item_num)
{
    ITEM *const item = &m_Items[item_num];
    if (Object_Get(item->object_id)->control == nullptr) {
        item->status = IS_INACTIVE;
        return;
    }

    if (item->active) {
        return;
    }

    item->active = 1;
    item->next_active = m_NextItemActive;
    m_NextItemActive = item_num;
}

void Item_NewRoom(const int16_t item_num, const int16_t room_num)
{
    ITEM *const item = &m_Items[item_num];
    ROOM *room = nullptr;

    if (item->room_num != NO_ROOM) {
        room = Room_Get(item->room_num);

        int16_t link_num = room->item_num;
        if (link_num == item_num) {
            room->item_num = item->next_item;
        } else {
            while (link_num != NO_ITEM) {
                if (m_Items[link_num].next_item == item_num) {
                    m_Items[link_num].next_item = item->next_item;
                    break;
                }
                link_num = m_Items[link_num].next_item;
            }
        }
    }

    room = Room_Get(room_num);
    item->room_num = room_num;
    item->next_item = room->item_num;
    room->item_num = item_num;
}

int32_t Item_GlobalReplace(
    const GAME_OBJECT_ID src_obj_id, const GAME_OBJECT_ID dst_obj_id)
{
    int32_t changed = 0;

    for (int32_t i = 0; i < Room_GetCount(); i++) {
        int16_t item_num = Room_Get(i)->item_num;
        while (item_num != NO_ITEM) {
            ITEM *const item = &m_Items[item_num];
            if (item->object_id == src_obj_id) {
                item->object_id = dst_obj_id;
                changed++;
            }
            item_num = item->next_item;
        }
    }

    return changed;
}

void Item_TakeDamage(
    ITEM *const item, const int16_t damage, const bool hit_status)
{
#if TR_VERSION == 1
    if (item->hit_points == DONT_TARGET) {
        return;
    }
#endif

    item->hit_points -= damage;
    CLAMPL(item->hit_points, 0);

    if (hit_status) {
        item->hit_status = 1;
    }
}

ITEM *Item_Find(const GAME_OBJECT_ID obj_id)
{
    for (int32_t item_num = 0; item_num < Item_GetTotalCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id == obj_id) {
            return item;
        }
    }

    return nullptr;
}

ANIM *Item_GetAnim(const ITEM *const item)
{
    return Anim_GetAnim(item->anim_num);
}

bool Item_TestAnimEqual(const ITEM *const item, const int16_t anim_idx)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    return item->anim_num == obj->anim_idx + anim_idx;
}

int16_t Item_GetRelativeAnim(const ITEM *const item)
{
    return Item_GetRelativeObjAnim(item, item->object_id);
}

int16_t Item_GetRelativeObjAnim(
    const ITEM *const item, const GAME_OBJECT_ID obj_id)
{
    return item->anim_num - Object_Get(obj_id)->anim_idx;
}

int16_t Item_GetRelativeFrame(const ITEM *const item)
{
    return item->frame_num - Item_GetAnim(item)->frame_base;
}

void Item_SwitchToAnim(
    ITEM *const item, const int16_t anim_idx, const int16_t frame)
{
    Item_SwitchToObjAnim(item, anim_idx, frame, item->object_id);
}

void Item_SwitchToObjAnim(
    ITEM *const item, const int16_t anim_idx, const int16_t frame,
    const GAME_OBJECT_ID obj_id)
{
    const OBJECT *const obj = Object_Get(obj_id);
    item->anim_num = obj->anim_idx + anim_idx;

    const ANIM *const anim = Item_GetAnim(item);
    if (frame < 0) {
        item->frame_num = anim->frame_end + frame + 1;
    } else {
        item->frame_num = anim->frame_base + frame;
    }
}

bool Item_TestFrameEqual(const ITEM *const item, const int16_t frame)
{
    return Anim_TestAbsFrameEqual(
        item->frame_num, Item_GetAnim(item)->frame_base + frame);
}

bool Item_TestFrameRange(
    const ITEM *const item, const int16_t start, const int16_t end)
{
    return Anim_TestAbsFrameRange(
        item->frame_num, Item_GetAnim(item)->frame_base + start,
        Item_GetAnim(item)->frame_base + end);
}

bool Item_GetAnimChange(ITEM *const item, const ANIM *const anim)
{
    if (item->current_anim_state == item->goal_anim_state) {
        return false;
    }

    for (int32_t i = 0; i < anim->num_changes; i++) {
        const ANIM_CHANGE *const change = Anim_GetChange(anim->change_idx + i);
        if (change->goal_anim_state != item->goal_anim_state) {
            continue;
        }

        for (int32_t j = 0; j < change->num_ranges; j++) {
            const ANIM_RANGE *const range =
                Anim_GetRange(change->range_idx + j);

            if (Anim_TestAbsFrameRange(
                    item->frame_num, range->start_frame, range->end_frame)) {
                item->anim_num = range->link_anim_num;
                item->frame_num = range->link_frame_num;
                return true;
            }
        }
    }

    return false;
}

void Item_Translate(
    ITEM *const item, const int32_t x, const int32_t y, const int32_t z)
{
    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    item->pos.x += ((c * x + s * z) >> W2V_SHIFT);
    item->pos.y += y;
    item->pos.z += ((c * z - s * x) >> W2V_SHIFT);
}

void Item_Animate(ITEM *const item)
{
    item->hit_status = 0;
    item->touch_bits = 0;
    item->frame_num++;

    const ANIM *anim = Item_GetAnim(item);
    if (anim->num_changes > 0 && Item_GetAnimChange(item, anim)) {
        anim = Item_GetAnim(item);
        item->current_anim_state = anim->current_anim_state;

        if (item->required_anim_state == anim->current_anim_state) {
            item->required_anim_state = 0;
        }
    }

    if (item->frame_num > anim->frame_end) {
        for (int32_t i = 0; i < anim->num_commands; i++) {
            const ANIM_COMMAND *const command = &anim->commands[i];
            switch (command->type) {
            case AC_MOVE_ORIGIN: {
                const XYZ_16 *const pos = (XYZ_16 *)command->data;
                Item_Translate(item, pos->x, pos->y, pos->z);
                break;
            }

            case AC_JUMP_VELOCITY: {
                const ANIM_COMMAND_VELOCITY_DATA *const data =
                    (ANIM_COMMAND_VELOCITY_DATA *)command->data;
                item->fall_speed = data->fall_speed;
                item->speed = data->speed;
                item->gravity = true;
                break;
            }

            case AC_DEACTIVATE:
                item->status = IS_DEACTIVATED;
                break;

            default:
                break;
            }
        }

        item->anim_num = anim->jump_anim_num;
        item->frame_num = anim->jump_frame_num;
        anim = Item_GetAnim(item);

#if TR_VERSION == 1
        item->current_anim_state = anim->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
#else
        if (item->current_anim_state != anim->current_anim_state) {
            item->current_anim_state = anim->current_anim_state;
            item->goal_anim_state = anim->current_anim_state;
        }
#endif

        if (item->required_anim_state == item->current_anim_state) {
            item->required_anim_state = 0;
        }
    }

    for (int32_t i = 0; i < anim->num_commands; i++) {
        const ANIM_COMMAND *const command = &anim->commands[i];
        switch (command->type) {
        case AC_SOUND_FX: {
            const ANIM_COMMAND_EFFECT_DATA *const data =
                (ANIM_COMMAND_EFFECT_DATA *)command->data;
            Item_PlayAnimSFX(item, data);
            break;
        }

        case AC_EFFECT:
            const ANIM_COMMAND_EFFECT_DATA *const data =
                (ANIM_COMMAND_EFFECT_DATA *)command->data;
            if (item->frame_num == data->frame_num) {
                ItemAction_Run(data->effect_num, item);
            }
            break;

        default:
            break;
        }
    }

    if (item->gravity != 0) {
        item->fall_speed += item->fall_speed < FAST_FALL_SPEED ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    } else {
        int32_t speed = anim->velocity;
        if (anim->acceleration != 0) {
            speed += anim->acceleration * (item->frame_num - anim->frame_base);
        }
        item->speed = speed >> 16;
    }

    item->pos.x += (item->speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.z += (item->speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;
}

void Item_PlayAnimSFX(
    const ITEM *const item, const ANIM_COMMAND_EFFECT_DATA *const data)
{
    if (item->frame_num != data->frame_num) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const bool item_underwater =
        (Room_Get(item->room_num)->flags & RF_UNDERWATER) != 0;
    const ANIM_COMMAND_ENVIRONMENT mode = data->environment;

    if (mode != ACE_ALL && item->room_num != NO_ROOM) {
        int32_t height = NO_HEIGHT;
        if (item == lara_item) {
            height = Lara_GetLaraInfo()->water_surface_dist;
        } else if (item_underwater) {
            height = -STEP_L;
        }

        if ((mode == ACE_WATER && (height >= 0 || height == NO_HEIGHT))
            || (mode == ACE_LAND && height < 0 && height != NO_HEIGHT)) {
            return;
        }
    }

    SOUND_PLAY_MODE play_mode = SPM_NORMAL;
    if (item == lara_item) {
        play_mode = SPM_ALWAYS;
    } else if (
        Object_IsType(item->object_id, g_WaterObjects)
        || (TR_VERSION == 1 && item_underwater)) {
        play_mode = SPM_UNDERWATER;
    }
#if TR_VERSION > 1
    else if (item->object_id == O_LARA_HARPOON) {
        play_mode = SPM_ALWAYS;
    }
#endif

    Sound_Effect(data->effect_num, &item->pos, play_mode);
}
