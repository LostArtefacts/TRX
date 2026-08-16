#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/input.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/lara/rope.h>
#include <trx/game/objects.h>
#include <trx/game/rope.h>

#include <string.h>

#define M_REACH_GRAB_RADIUS 128
#define M_JUMP_GRAB_RADIUS 320
#define M_GRAB_Y_OFFSET 512
#define M_LF_SWING_NEUTRAL 32
#define M_LF_SWING_FORWARD 60

static void M_Initialise(const int16_t item_num)
{
    Rope_Create(item_num);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    ROPE *const rope = Rope_Get(Rope_GetIndexByItem(item_num));
    if (rope == nullptr) {
        return;
    }

    if (Item_IsTriggerActive(item)) {
        rope->active = true;
        memcpy(
            rope->prev_mesh_segments, rope->mesh_segments,
            sizeof(rope->prev_mesh_segments));
        Rope_Calculate(rope);
    } else {
        rope->active = false;
    }
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    const int32_t rope_num = Rope_GetIndexByItem(item_num);
    ROPE *const rope = Rope_Get(rope_num);
    if (rope == nullptr) {
        return;
    }

    const bool is_reach = lara_item->current_anim_state == LS(LS_REACH);
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || (!is_reach && lara_item->current_anim_state != LS(LS_JUMP_UP))
        || !lara_item->gravity || lara_item->fall_speed <= 0 || !rope->active) {
        return;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(lara_item);
    XYZ_32 test_pos = lara_item->pos;
    test_pos.y += bounds->min.y + M_GRAB_Y_OFFSET;
    test_pos = XYZ_32_OffsetYaw(test_pos, lara_item->rot.y, bounds->max.z);
    const int32_t radius = is_reach ? M_REACH_GRAB_RADIUS : M_JUMP_GRAB_RADIUS;

    const int32_t segment = Rope_NodeCollision(rope, test_pos, radius);
    if (segment < 0) {
        return;
    }

    if (is_reach) {
        Item_SwitchToAnim(lara_item, LA(LA_ROPE_JUMP_TO_GRAB), 0);
        lara_item->current_anim_state = LS(LS_ROPE_FORWARD);
        const int32_t frame_base = Lara_Rope_GetSwingAnim()->frame_base;
        lara->rope.frame = (frame_base + M_LF_SWING_NEUTRAL) << 8;
        lara->rope.d_frame = (frame_base + M_LF_SWING_FORWARD) << 8;
    } else {
        Item_SwitchToAnim(lara_item, LA(LA_JUMP_UP_TO_ROPE_START), 0);
        lara_item->current_anim_state = LS(LS_ROPE_IDLE);
    }

    lara_item->gravity = false;
    lara_item->fall_speed = 0;
    lara->gun_status = LGS_HANDS_BUSY;
    lara->rope.index = rope_num;
    lara->rope.segment = segment;
    lara->rope.y_rot = lara_item->rot.y;
    Rope_AlignLara(lara_item);
    Rope_GetPendulum()->vel = (XYZ_32) {};
    Lara_Rope_ApplyVelocity(lara_item->rot.y, lara_item->speed << 4);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_ROPE, M_Setup)
