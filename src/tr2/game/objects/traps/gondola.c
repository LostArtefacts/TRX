#include "game/objects/traps/gondola.h"

#include "game/items.h"
#include "game/objects/common.h"
#include "game/room.h"

#define GONDOLA_SINK_SPEED 50

void Gondola_Control(const int16_t item_num)
{
    ITEM *const gondola = Item_Get(item_num);

    switch (gondola->current_anim_state) {
    case GONDOLA_STATE_FLOATING:
        if (gondola->goal_anim_state == GONDOLA_STATE_CRASH) {
            gondola->mesh_bits = 0xFF;
            Item_Explode(item_num, 240, 0);
        }
        break;

    case GONDOLA_STATE_SINK: {
        gondola->pos.y = gondola->pos.y + GONDOLA_SINK_SPEED;
        int16_t room_num = gondola->room_num;
        const SECTOR *const sector = Room_GetSector(
            gondola->pos.x, gondola->pos.y, gondola->pos.z, &room_num);
        const int32_t height = Room_GetHeight(
            sector, gondola->pos.x, gondola->pos.y, gondola->pos.z);
        gondola->floor = height;

        if (gondola->pos.y >= height) {
            gondola->goal_anim_state = GONDOLA_STATE_LAND;
            gondola->pos.y = height;
        }
        break;
    }
    }

    Item_Animate(gondola);

    if (gondola->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}

void Gondola_Setup(void)
{
    OBJECT *const obj = Object_Get(O_GONDOLA);
    obj->control_func = Gondola_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = 1;
    obj->save_anim = 1;
}
