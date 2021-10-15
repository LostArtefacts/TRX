#include "game/control.h"

#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/camera.h"
#include "game/demo.h"
#include "game/hair.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/mnsound.h"
#include "game/objects/keyhole.h"
#include "game/objects/puzzle_hole.h"
#include "game/objects/switch.h"
#include "game/pause.h"
#include "game/sound.h"
#include "game/traps/lava.h"
#include "game/traps/movable_block.h"
#include "global/const.h"
#include "global/vars.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/sndpc.h"
#include "util.h"

#include <stddef.h>

static const int32_t AnimationRate = 0x8000;

void CheckCheatMode()
{
    static int32_t cheat_mode = 0;
    static int16_t cheat_angle = 0;
    static int32_t cheat_turn = 0;

    if (CurrentLevel == GF.gym_level_num) {
        return;
    }

    LARA_STATE as = LaraItem->current_anim_state;
    switch (cheat_mode) {
    case 0:
        if (as == AS_WALK) {
            cheat_mode = 1;
        }
        break;

    case 1:
        if (as != AS_WALK) {
            cheat_mode = as == AS_STOP ? 2 : 0;
        }
        break;

    case 2:
        if (as != AS_STOP) {
            cheat_mode = as == AS_BACK ? 3 : 0;
        }
        break;

    case 3:
        if (as != AS_BACK) {
            cheat_mode = as == AS_STOP ? 4 : 0;
        }
        break;

    case 4:
        if (as != AS_STOP) {
            cheat_angle = LaraItem->pos.y_rot;
        }
        cheat_turn = 0;
        if (as == AS_TURN_L) {
            cheat_mode = 5;
        } else if (as == AS_TURN_R) {
            cheat_mode = 6;
        } else {
            cheat_mode = 0;
        }
        break;

    case 5:
        if (as == AS_TURN_L || as == AS_FASTTURN) {
            cheat_turn += (int16_t)(LaraItem->pos.y_rot - cheat_angle);
            cheat_angle = LaraItem->pos.y_rot;
        } else {
            cheat_mode = cheat_turn < -94208 ? 7 : 0;
        }
        break;

    case 6:
        if (as == AS_TURN_R || as == AS_FASTTURN) {
            cheat_turn += (int16_t)(LaraItem->pos.y_rot - cheat_angle);
            cheat_angle = LaraItem->pos.y_rot;
        } else {
            cheat_mode = cheat_turn > 94208 ? 7 : 0;
        }
        break;

    case 7:
        if (as != AS_STOP) {
            cheat_mode = as == AS_COMPRESS ? 8 : 0;
        }
        break;

    case 8:
        if (LaraItem->fall_speed > 0) {
            if (as == AS_FORWARDJUMP) {
                LevelComplete = 1;
            } else if (as == AS_BACKJUMP) {
                Inv_AddItem(O_SHOTGUN_ITEM);
                Inv_AddItem(O_MAGNUM_ITEM);
                Inv_AddItem(O_UZI_ITEM);
                Lara.shotgun.ammo = 500;
                Lara.magnums.ammo = 500;
                Lara.uzis.ammo = 5000;
                SoundEffect(SFX_LARA_HOLSTER, NULL, SPM_ALWAYS);
            }
            cheat_mode = 0;
        }
        break;

    default:
        cheat_mode = 0;
        break;
    }
}

int32_t ControlPhase(int32_t nframes, int32_t demo_mode)
{
    static int32_t frame_count = 0;
    int32_t return_val = 0;
    if (nframes > MAX_FRAMES) {
        nframes = MAX_FRAMES;
    }

    frame_count += AnimationRate * nframes;
    while (frame_count >= 0) {
        if (CDTrack > 0) {
            S_MusicLoop();
        }

        CheckCheatMode();
        if (LevelComplete) {
            return GF_NOP_BREAK;
        }

        S_UpdateInput();

        if (ResetFlag) {
            return GF_NOP_BREAK;
        }

        if (demo_mode) {
            if (Input) {
                return GF_EXIT_TO_TITLE;
            }
            GetDemoInput();
            if (Input == -1) {
                return GF_EXIT_TO_TITLE;
            }
        }

        if (Lara.death_count > DEATH_WAIT
            || (Lara.death_count > DEATH_WAIT_MIN && (Input & ~IN_FLY_CHEAT))
            || OverlayFlag == 2) {
            if (demo_mode) {
                return GF_EXIT_TO_TITLE;
            }
            if (OverlayFlag == 2) {
                OverlayFlag = 1;
                return_val = Display_Inventory(INV_DEATH_MODE);
                if (return_val != GF_NOP) {
                    return return_val;
                }
            } else {
                OverlayFlag = 2;
            }
        }

        if ((Input & (IN_OPTION | IN_SAVE | IN_LOAD) || OverlayFlag <= 0)
            && !Lara.death_count) {
            if (OverlayFlag > 0) {
                if (Input & IN_LOAD) {
                    OverlayFlag = -1;
                } else if (Input & IN_SAVE) {
                    OverlayFlag = -2;
                } else {
                    OverlayFlag = 0;
                }
            } else {
                if (OverlayFlag == -1) {
                    return_val = Display_Inventory(INV_LOAD_MODE);
                } else if (OverlayFlag == -2) {
                    return_val = Display_Inventory(INV_SAVE_MODE);
                } else {
                    return_val = Display_Inventory(INV_GAME_MODE);
                }

                OverlayFlag = 1;
                if (return_val != GF_NOP) {
                    return return_val;
                }
            }
        }

        if (!Lara.death_count && CHK_ANY(GetDebouncedInput(Input), IN_PAUSE)) {
            if (S_Pause()) {
                return GF_EXIT_TO_TITLE;
            }
        }

        int16_t item_num = NextItemActive;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &Items[item_num];
            OBJECT_INFO *obj = &Objects[item->object_number];
            if (obj->control) {
                obj->control(item_num);
            }
            item_num = item->next_active;
        }

        item_num = NextFxActive;
        while (item_num != NO_ITEM) {
            FX_INFO *fx = &Effects[item_num];
            OBJECT_INFO *obj = &Objects[fx->object_number];
            if (obj->control) {
                obj->control(item_num);
            }
            item_num = fx->next_active;
        }

        LaraControl(0);
        HairControl(0);

        CalculateCamera();
        SoundEffects();
        SaveGame.timer++;
        HealthBarTimer--;

        if (T1MConfig.disable_healing_between_levels) {
            int8_t lara_found = 0;
            for (int i = 0; i < LevelItemCount; i++) {
                if (Items[i].object_number == O_LARA) {
                    lara_found = 1;
                }
            }
            if (lara_found) {
                StoredLaraHealth =
                    LaraItem ? LaraItem->hit_points : LARA_HITPOINTS;
            }
        }

        frame_count -= 0x10000;
    }

    return GF_NOP;
}

void AnimateItem(ITEM_INFO *item)
{
    item->touch_bits = 0;
    item->hit_status = 0;

    ANIM_STRUCT *anim = &Anims[item->anim_number];

    item->frame_number++;

    if (anim->number_changes > 0) {
        if (GetChange(item, anim)) {
            anim = &Anims[item->anim_number];
            item->current_anim_state = anim->current_anim_state;

            if (item->required_anim_state == item->current_anim_state) {
                item->required_anim_state = 0;
            }
        }
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            int16_t *command = &AnimCommands[anim->command_index];
            for (int i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    TranslateItem(item, command[0], command[1], command[2]);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    item->fall_speed = command[0];
                    item->speed = command[1] / AnimScale;
                    item->gravity_status = 1;
                    command += 2;
                    break;

                case AC_DEACTIVATE:
                    item->status = IS_DEACTIVATED;
                    break;

                case AC_SOUND_FX:
                case AC_EFFECT:
                    command += 2;
                    break;
                }
            }
        }

        item->anim_number = anim->jump_anim_num;
        item->frame_number = anim->jump_frame_num;

        anim = &Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        if (item->required_anim_state == item->current_anim_state) {
            item->required_anim_state = 0;
        }
    }

    if (anim->number_commands > 0) {
        int16_t *command = &AnimCommands[anim->command_index];
        for (int i = 0; i < anim->number_commands; i++) {
            switch (*command++) {
            case AC_MOVE_ORIGIN:
                command += 3;
                break;

            case AC_JUMP_VELOCITY:
                command += 2;
                break;

            case AC_SOUND_FX:
                if (item->frame_number == command[0]) {
                    SoundEffect(
                        command[1], &item->pos,
                        RoomInfo[item->room_number].flags);
                }
                command += 2;
                break;

            case AC_EFFECT:
                if (item->frame_number == command[0]) {
                    EffectRoutines[command[1]](item);
                }
                command += 2;
                break;
            }
        }
    }

    if (!item->gravity_status) {
        int32_t speed = anim->velocity / AnimScale;
        if (anim->acceleration) {
            speed += anim->acceleration / AnimScale
                * ((item->frame_number - anim->frame_base) / AnimScale);
        }
        item->speed = speed >> 16;
    } else {
        item->fall_speed +=
            (item->fall_speed < FASTFALL_SPEED) ? GRAVITY / AnimScale : 1;
        item->pos.y += item->fall_speed / AnimScale;
    }

    item->pos.x += (phd_sin(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
    item->pos.z += (phd_cos(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
}

int32_t GetChange(ITEM_INFO *item, ANIM_STRUCT *anim)
{
    if (item->current_anim_state == item->goal_anim_state) {
        return 0;
    }

    ANIM_CHANGE_STRUCT *change = &AnimChanges[anim->change_index];
    for (int i = 0; i < anim->number_changes; i++, change++) {
        if (change->goal_anim_state == item->goal_anim_state) {
            ANIM_RANGE_STRUCT *range = &AnimRanges[change->range_index];
            for (int j = 0; j < change->number_ranges; j++, range++) {
                if (item->frame_number >= range->start_frame
                    && item->frame_number <= range->end_frame) {
                    item->anim_number = range->link_anim_num;
                    item->frame_number = range->link_frame_num;
                    return 1;
                }
            }
        }
    }

    return 0;
}

void TranslateItem(ITEM_INFO *item, int32_t x, int32_t y, int32_t z)
{
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);

    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}

void TranslateItem_f(
    PHD_3DPOS_F *item, double x, double y, double z, int16_t y_rot)
{
    double c = phd_cos_f(y_rot);
    double s = phd_sin_f(y_rot);

    item->x += (c * x + s * z) / View2World;
    item->y += y;
    item->z += (c * z - s * x) / View2World;
}

FLOOR_INFO *GetFloor(int32_t x, int32_t y, int32_t z, int16_t *room_num)
{
    int16_t data;
    FLOOR_INFO *floor;
    ROOM_INFO *r = &RoomInfo[*room_num];
    do {
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;

        if (x_floor <= 0) {
            x_floor = 0;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (x_floor >= r->x_size - 1) {
            x_floor = r->x_size - 1;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (y_floor < 0) {
            y_floor = 0;
        } else if (y_floor >= r->y_size) {
            y_floor = r->y_size - 1;
        }

        floor = &r->floor[x_floor + y_floor * r->x_size];
        if (!floor->index) {
            break;
        }

        data = GetDoor(floor);
        if (data != NO_ROOM) {
            *room_num = data;
            r = &RoomInfo[data];
        }
    } while (data != NO_ROOM);

    if (y >= ((int32_t)floor->floor << 8)) {
        do {
            if (floor->pit_room == NO_ROOM) {
                break;
            }

            *room_num = floor->pit_room;

            r = &RoomInfo[floor->pit_room];
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        } while (y >= ((int32_t)floor->floor << 8));
    } else if (y < ((int32_t)floor->ceiling << 8)) {
        do {
            if (floor->sky_room == NO_ROOM) {
                break;
            }

            *room_num = floor->sky_room;

            r = &RoomInfo[floor->sky_room];
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        } while (y < ((int32_t)floor->ceiling << 8));
    }

    return floor;
}

int16_t GetWaterHeight(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    ROOM_INFO *r = &RoomInfo[room_num];

    int16_t data;
    FLOOR_INFO *floor;
    int32_t x_floor, y_floor;

    do {
        x_floor = (z - r->z) >> WALL_SHIFT;
        y_floor = (x - r->x) >> WALL_SHIFT;

        if (x_floor <= 0) {
            x_floor = 0;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (x_floor >= r->x_size - 1) {
            x_floor = r->x_size - 1;
            if (y_floor < 1) {
                y_floor = 1;
            } else if (y_floor > r->y_size - 2) {
                y_floor = r->y_size - 2;
            }
        } else if (y_floor < 0) {
            y_floor = 0;
        } else if (y_floor >= r->y_size) {
            y_floor = r->y_size - 1;
        }

        floor = &r->floor[x_floor + y_floor * r->x_size];
        data = GetDoor(floor);
        if (data != NO_ROOM) {
            r = &RoomInfo[data];
        }
    } while (data != NO_ROOM);

    if (r->flags & RF_UNDERWATER) {
        while (floor->sky_room != NO_ROOM) {
            r = &RoomInfo[floor->sky_room];
            if (!(r->flags & RF_UNDERWATER)) {
                break;
            }
            x_floor = (z - r->z) >> WALL_SHIFT;
            y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return floor->ceiling << 8;
    } else {
        while (floor->pit_room != NO_ROOM) {
            r = &RoomInfo[floor->pit_room];
            if (r->flags & RF_UNDERWATER) {
                return floor->floor << 8;
            }
            x_floor = (z - r->z) >> WALL_SHIFT;
            y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return NO_HEIGHT;
    }
}

int16_t GetHeight(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z)
{
    HeightType = HT_WALL;
    while (floor->pit_room != NO_ROOM) {
        ROOM_INFO *r = &RoomInfo[floor->pit_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        floor = &r->floor[x_floor + y_floor * r->x_size];
    }

    int16_t height = floor->floor << 8;

    TriggerIndex = NULL;

    if (!floor->index) {
        return height;
    }

    int16_t *data = &FloorData[floor->index];
    int16_t type;
    int16_t trigger;
    do {
        type = *data++;

        switch (type & DATA_TYPE) {
        case FT_TILT: {
            int16_t xoff = data[0] >> 8;
            int16_t yoff = (int8_t)data[0];

            if (!ChunkyFlag || (ABS(xoff) <= 2 && ABS(yoff) <= 2)) {
                if (ABS(xoff) > 2 || ABS(yoff) > 2) {
                    HeightType = HT_BIG_SLOPE;
                } else {
                    HeightType = HT_SMALL_SLOPE;
                }

                if (xoff < 0) {
                    height -= (int16_t)((xoff * (z & (WALL_L - 1))) >> 2);
                } else {
                    height += (int16_t)(
                        (xoff * ((WALL_L - 1 - z) & (WALL_L - 1))) >> 2);
                }

                if (yoff < 0) {
                    height -= (int16_t)((yoff * (x & (WALL_L - 1))) >> 2);
                } else {
                    height += (int16_t)(
                        (yoff * ((WALL_L - 1 - x) & (WALL_L - 1))) >> 2);
                }
            }

            data++;
            break;
        }

        case FT_ROOF:
        case FT_DOOR:
            data++;
            break;

        case FT_LAVA:
            TriggerIndex = data - 1;
            break;

        case FT_TRIGGER:
            if (!TriggerIndex) {
                TriggerIndex = data - 1;
            }

            data++;
            do {
                trigger = *data++;
                if (TRIG_BITS(trigger) != TO_OBJECT) {
                    if (TRIG_BITS(trigger) == TO_CAMERA) {
                        trigger = *data++;
                    }
                } else {
                    ITEM_INFO *item = &Items[trigger & VALUE_BITS];
                    OBJECT_INFO *object = &Objects[item->object_number];
                    if (object->floor) {
                        object->floor(item, x, y, z, &height);
                    }
                }
            } while (!(trigger & END_BIT));
            break;

        default:
            S_ExitSystem("GetHeight(): Unknown type");
            break;
        }
    } while (!(type & END_BIT));

    return height;
}

void RefreshCamera(int16_t type, int16_t *data)
{
    int16_t trigger;
    int16_t target_ok = 2;
    do {
        trigger = *data++;
        int16_t value = trigger & VALUE_BITS;

        switch (TRIG_BITS(trigger)) {
        case TO_CAMERA:
            data++;

            if (value == Camera.last) {
                Camera.number = value;

                if (Camera.timer < 0 || Camera.type == CAM_LOOK
                    || Camera.type == CAM_COMBAT) {
                    Camera.timer = -1;
                    target_ok = 0;
                } else {
                    Camera.type = CAM_FIXED;
                    target_ok = 1;
                }
            } else {
                target_ok = 0;
            }
            break;

        case TO_TARGET:
            if (Camera.type != CAM_LOOK && Camera.type != CAM_COMBAT) {
                Camera.item = &Items[value];
            }
            break;
        }
    } while (!(trigger & END_BIT));

    if (Camera.item != NULL) {
        if (!target_ok
            || (target_ok == 2 && Camera.item->looked_at
                && Camera.item != Camera.last_item)) {
            Camera.item = NULL;
        }
    }
}

void TestTriggers(int16_t *data, int32_t heavy)
{
    if (!data) {
        return;
    }

    if ((*data & DATA_TYPE) == FT_LAVA) {
        if (!heavy && LaraItem->pos.y == LaraItem->floor) {
            LavaBurn(LaraItem);
        }

        if (*data & END_BIT) {
            return;
        }

        data++;
    }

    int16_t type = (*data++ >> 8) & 0x3F;
    int32_t switch_off = 0;
    int32_t flip = 0;
    int32_t new_effect = -1;
    int16_t flags = *data++;
    int16_t timer = flags & 0xFF;

    if (Camera.type != CAM_HEAVY) {
        RefreshCamera(type, data);
    }

    if (heavy) {
        if (type != TT_HEAVY) {
            return;
        }
    } else {
        switch (type) {
        case TT_SWITCH: {
            int16_t value = *data++ & VALUE_BITS;
            if (!SwitchTrigger(value, timer)) {
                return;
            }
            switch_off = Items[value].current_anim_state == AS_RUN;
            break;
        }

        case TT_PAD:
        case TT_ANTIPAD:
            if (LaraItem->pos.y != LaraItem->floor) {
                return;
            }
            break;

        case TT_KEY: {
            int16_t value = *data++ & VALUE_BITS;
            if (!KeyTrigger(value)) {
                return;
            }
            break;
        }

        case TT_PICKUP: {
            int16_t value = *data++ & VALUE_BITS;
            if (!PickupTrigger(value)) {
                return;
            }
            break;
        }

        case TT_HEAVY:
        case TT_DUMMY:
            return;

        case TT_COMBAT:
            if (Lara.gun_status != LGS_READY) {
                return;
            }
            break;
        }
    }

    ITEM_INFO *camera_item = NULL;
    int16_t trigger;
    do {
        trigger = *data++;
        int16_t value = trigger & VALUE_BITS;

        switch (TRIG_BITS(trigger)) {
        case TO_OBJECT: {
            ITEM_INFO *item = &Items[value];

            if (item->flags & IF_ONESHOT) {
                break;
            }

            item->timer = timer;
            if (timer != 1) {
                item->timer *= FRAMES_PER_SECOND;
            }

            if (type == TT_SWITCH) {
                item->flags ^= flags & IF_CODE_BITS;
            } else if (type == TT_ANTIPAD) {
                item->flags &= -1 - (flags & IF_CODE_BITS);
            } else if (flags & IF_CODE_BITS) {
                item->flags |= flags & IF_CODE_BITS;
            }

            if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
                break;
            }

            if (flags & IF_ONESHOT) {
                item->flags |= IF_ONESHOT;
            }

            if (!item->active) {
                if (Objects[item->object_number].intelligent) {
                    if (item->status == IS_NOT_ACTIVE) {
                        item->touch_bits = 0;
                        item->status = IS_ACTIVE;
                        AddActiveItem(value);
                        EnableBaddieAI(value, 1);
                    } else if (item->status == IS_INVISIBLE) {
                        item->touch_bits = 0;
                        if (EnableBaddieAI(value, 0)) {
                            item->status = IS_ACTIVE;
                        } else {
                            item->status = IS_INVISIBLE;
                        }
                        AddActiveItem(value);
                    }
                } else {
                    item->touch_bits = 0;
                    item->status = IS_ACTIVE;
                    AddActiveItem(value);
                }
            }
            break;
        }

        case TO_CAMERA: {
            trigger = *data++;
            int16_t camera_flags = trigger;
            int16_t camera_timer = trigger & 0xFF;

            if (Camera.fixed[value].flags & IF_ONESHOT) {
                break;
            }

            Camera.number = value;

            if (Camera.type == CAM_LOOK || Camera.type == CAM_COMBAT) {
                break;
            }

            if (type == TT_COMBAT) {
                break;
            }

            if (type == TT_SWITCH && timer && switch_off) {
                break;
            }

            if (Camera.number == Camera.last && type != TT_SWITCH) {
                break;
            }

            Camera.timer = camera_timer;
            if (Camera.timer != 1) {
                Camera.timer *= FRAMES_PER_SECOND;
            }

            if (camera_flags & IF_ONESHOT) {
                Camera.fixed[Camera.number].flags |= IF_ONESHOT;
            }

            Camera.speed = ((camera_flags & IF_CODE_BITS) >> 6) + 1;
            Camera.type = heavy ? CAM_HEAVY : CAM_FIXED;
            break;
        }

        case TO_TARGET:
            camera_item = &Items[value];
            break;

        case TO_SINK: {
            OBJECT_VECTOR *obvector = &Camera.fixed[value];

            if (Lara.LOT.required_box != obvector->flags) {
                Lara.LOT.target.x = obvector->x;
                Lara.LOT.target.y = obvector->y;
                Lara.LOT.target.z = obvector->z;
                Lara.LOT.required_box = obvector->flags;
            }

            Lara.current_active = obvector->data * 6 / AnimScale;
            break;
        }

        case TO_FLIPMAP:
            if (FlipMapTable[value] & IF_ONESHOT) {
                break;
            }

            if (type == TT_SWITCH) {
                FlipMapTable[value] ^= flags & IF_CODE_BITS;
            } else if (flags & IF_CODE_BITS) {
                FlipMapTable[value] |= flags & IF_CODE_BITS;
            }

            if ((FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS) {
                if (flags & IF_ONESHOT) {
                    FlipMapTable[value] |= IF_ONESHOT;
                }

                if (!FlipStatus) {
                    flip = 1;
                }
            } else if (FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPON:
            if ((FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS
                && !FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPOFF:
            if ((FlipMapTable[value] & IF_CODE_BITS) == IF_CODE_BITS
                && FlipStatus) {
                flip = 1;
            }
            break;

        case TO_FLIPEFFECT:
            new_effect = value;
            break;

        case TO_FINISH:
            LevelComplete = 1;
            break;

        case TO_CD:
            TriggerCDTrack(value, flags, type);
            break;

        case TO_SECRET:
            if ((SaveGame.secrets & (1 << value))) {
                break;
            }
            SaveGame.secrets |= 1 << value;
            S_MusicPlay(13);
            break;
        }
    } while (!(trigger & END_BIT));

    if (camera_item && (Camera.type == CAM_FIXED || Camera.type == CAM_HEAVY)) {
        Camera.item = camera_item;
    }

    if (flip) {
        FlipMap();
        if (new_effect != -1) {
            FlipEffect = new_effect;
            FlipTimer = 0;
        }
    }
}

int32_t TriggerActive(ITEM_INFO *item)
{
    int32_t ok = (item->flags & IF_REVERSE) ? 0 : 1;

    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return !ok;
    }

    if (!item->timer) {
        return ok;
    }

    if (item->timer == -1) {
        return !ok;
    }

    item->timer--;

    if (!item->timer) {
        item->timer = -1;
    }

    return ok;
}

int16_t GetCeiling(FLOOR_INFO *floor, int32_t x, int32_t y, int32_t z)
{
    int16_t *data;
    int16_t type;
    int16_t trigger;

    FLOOR_INFO *f = floor;
    while (f->sky_room != NO_ROOM) {
        ROOM_INFO *r = &RoomInfo[f->sky_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        f = &r->floor[x_floor + y_floor * r->x_size];
    }

    int16_t height = f->ceiling << 8;

    if (f->index) {
        data = &FloorData[f->index];
        type = *data++ & DATA_TYPE;

        if (type == FT_TILT) {
            data++;
            type = *data++ & DATA_TYPE;
        }

        if (type == FT_ROOF) {
            int32_t xoff = data[0] >> 8;
            int32_t yoff = (int8_t)data[0];

            if (!ChunkyFlag
                || (xoff >= -2 && xoff <= 2 && yoff >= -2 && yoff <= 2)) {
                if (xoff < 0) {
                    height += (int16_t)((xoff * (z & (WALL_L - 1))) >> 2);
                } else {
                    height -= (int16_t)(
                        (xoff * ((WALL_L - 1 - z) & (WALL_L - 1))) >> 2);
                }

                if (yoff < 0) {
                    height += (int16_t)(
                        (yoff * ((WALL_L - 1 - x) & (WALL_L - 1))) >> 2);
                } else {
                    height -= (int16_t)((yoff * (x & (WALL_L - 1))) >> 2);
                }
            }
        }
    }

    while (floor->pit_room != NO_ROOM) {
        ROOM_INFO *r = &RoomInfo[floor->pit_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        floor = &r->floor[x_floor + y_floor * r->x_size];
    }

    if (!floor->index) {
        return height;
    }

    data = &FloorData[floor->index];
    do {
        type = *data++;

        switch (type & DATA_TYPE) {
        case FT_DOOR:
        case FT_TILT:
        case FT_ROOF:
            data++;
            break;

        case FT_LAVA:
            break;

        case FT_TRIGGER:
            data++;
            do {
                trigger = *data++;
                if (TRIG_BITS(trigger) != TO_OBJECT) {
                    if (TRIG_BITS(trigger) == TO_CAMERA) {
                        trigger = *data++;
                    }
                } else {
                    ITEM_INFO *item = &Items[trigger & VALUE_BITS];
                    OBJECT_INFO *object = &Objects[item->object_number];
                    if (object->ceiling) {
                        object->ceiling(item, x, y, z, &height);
                    }
                }
            } while (!(trigger & END_BIT));
            break;

        default:
            S_ExitSystem("GetCeiling(): Unknown type");
            break;
        }
    } while (!(type & END_BIT));

    return height;
}

int16_t GetDoor(FLOOR_INFO *floor)
{
    if (!floor->index) {
        return NO_ROOM;
    }

    int16_t *data = &FloorData[floor->index];
    int16_t type = *data++;

    if (type == FT_TILT) {
        data++;
        type = *data++;
    }

    if (type == FT_ROOF) {
        data++;
        type = *data++;
    }

    if ((type & DATA_TYPE) == FT_DOOR) {
        return *data;
    }
    return NO_ROOM;
}

int32_t LOS(GAME_VECTOR *start, GAME_VECTOR *target)
{
    int32_t los1;
    int32_t los2;

    if (ABS(target->z - start->z) > ABS(target->x - start->x)) {
        los1 = xLOS(start, target);
        los2 = zLOS(start, target);
    } else {
        los1 = zLOS(start, target);
        los2 = xLOS(start, target);
    }

    if (!los2) {
        return 0;
    }

    FLOOR_INFO *floor =
        GetFloor(target->x, target->y, target->z, &target->room_number);

    if (ClipTarget(start, target, floor) && los1 == 1 && los2 == 1) {
        return 1;
    }

    return 0;
}

int32_t zLOS(GAME_VECTOR *start, GAME_VECTOR *target)
{
    FLOOR_INFO *floor;

    int32_t dz = target->z - start->z;
    if (dz == 0) {
        return 1;
    }

    int32_t dx = ((target->x - start->x) << WALL_SHIFT) / dz;
    int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dz;

    int16_t room_num = start->room_number;
    int16_t last_room;

    if (dz < 0) {
        int32_t z = start->z & ~(WALL_L - 1);
        int32_t x = start->x + ((dx * (z - start->z)) >> WALL_SHIFT);
        int32_t y = start->y + ((dy * (z - start->z)) >> WALL_SHIFT);

        while (z > target->z) {
            floor = GetFloor(x, y, z, &room_num);
            if (y > GetHeight(floor, x, y, z)
                || y < GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x, y, z - 1, &room_num);
            if (y > GetHeight(floor, x, y, z - 1)
                || y < GetCeiling(floor, x, y, z - 1)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
                return 0;
            }

            z -= WALL_L;
            x -= dx;
            y -= dy;
        }
    } else {
        int32_t z = start->z | (WALL_L - 1);
        int32_t x = start->x + ((dx * (z - start->z)) >> WALL_SHIFT);
        int32_t y = start->y + ((dy * (z - start->z)) >> WALL_SHIFT);

        while (z < target->z) {
            floor = GetFloor(x, y, z, &room_num);
            if (y > GetHeight(floor, x, y, z)
                || y < GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x, y, z + 1, &room_num);
            if (y > GetHeight(floor, x, y, z + 1)
                || y < GetCeiling(floor, x, y, z + 1)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
                return 0;
            }

            z += WALL_L;
            x += dx;
            y += dy;
        }
    }

    target->room_number = room_num;
    return 1;
}

int32_t xLOS(GAME_VECTOR *start, GAME_VECTOR *target)
{
    FLOOR_INFO *floor;

    int32_t dx = target->x - start->x;
    if (dx == 0) {
        return 1;
    }

    int32_t dy = ((target->y - start->y) << WALL_SHIFT) / dx;
    int32_t dz = ((target->z - start->z) << WALL_SHIFT) / dx;

    int16_t room_num = start->room_number;
    int16_t last_room;

    if (dx < 0) {
        int32_t x = start->x & ~(WALL_L - 1);
        int32_t y = start->y + ((dy * (x - start->x)) >> WALL_SHIFT);
        int32_t z = start->z + ((dz * (x - start->x)) >> WALL_SHIFT);

        while (x > target->x) {
            floor = GetFloor(x, y, z, &room_num);
            if (y > GetHeight(floor, x, y, z)
                || y < GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x - 1, y, z, &room_num);
            if (y > GetHeight(floor, x - 1, y, z)
                || y < GetCeiling(floor, x - 1, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
                return 0;
            }

            x -= WALL_L;
            y -= dy;
            z -= dz;
        }
    } else {
        int32_t x = start->x | (WALL_L - 1);
        int32_t y = start->y + ((dy * (x - start->x)) >> WALL_SHIFT);
        int32_t z = start->z + ((dz * (x - start->x)) >> WALL_SHIFT);

        while (x < target->x) {
            floor = GetFloor(x, y, z, &room_num);
            if (y > GetHeight(floor, x, y, z)
                || y < GetCeiling(floor, x, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = room_num;
                return -1;
            }

            last_room = room_num;

            floor = GetFloor(x + 1, y, z, &room_num);
            if (y > GetHeight(floor, x + 1, y, z)
                || y < GetCeiling(floor, x + 1, y, z)) {
                target->x = x;
                target->y = y;
                target->z = z;
                target->room_number = last_room;
                return 0;
            }

            x += WALL_L;
            y += dy;
            z += dz;
        }
    }

    target->room_number = room_num;
    return 1;
}

int32_t ClipTarget(GAME_VECTOR *start, GAME_VECTOR *target, FLOOR_INFO *floor)
{
    int32_t dx = target->x - start->x;
    int32_t dy = target->y - start->y;
    int32_t dz = target->z - start->z;

    int32_t height = GetHeight(floor, target->x, target->y, target->z);
    if (target->y > height && start->y < height) {
        target->y = height;
        target->x = start->x + dx * (height - start->y) / dy;
        target->z = start->z + dz * (height - start->y) / dy;
        return 0;
    }

    int32_t ceiling = GetCeiling(floor, target->x, target->y, target->z);
    if (target->y < ceiling && start->y > ceiling) {
        target->y = ceiling;
        target->x = start->x + dx * (ceiling - start->y) / dy;
        target->z = start->z + dz * (ceiling - start->y) / dy;
        return 0;
    }

    return 1;
}

void FlipMap()
{
    mn_stop_ambient_samples();

    for (int i = 0; i < RoomCount; i++) {
        ROOM_INFO *r = &RoomInfo[i];
        if (r->flipped_room < 0) {
            continue;
        }

        RemoveRoomFlipItems(r);

        ROOM_INFO *flipped = &RoomInfo[r->flipped_room];
        ROOM_INFO temp = *r;
        *r = *flipped;
        *flipped = temp;

        r->flipped_room = flipped->flipped_room;
        flipped->flipped_room = -1;

        // XXX: is this really necessary given the assignments above?
        r->item_number = flipped->item_number;
        r->fx_number = flipped->fx_number;

        AddRoomFlipItems(r);
    }

    FlipStatus = !FlipStatus;
}

void RemoveRoomFlipItems(ROOM_INFO *r)
{
    for (int16_t item_num = r->item_number; item_num != NO_ITEM;
         item_num = Items[item_num].next_item) {
        ITEM_INFO *item = &Items[item_num];

        switch (item->object_number) {
        case O_MOVABLE_BLOCK:
        case O_MOVABLE_BLOCK2:
        case O_MOVABLE_BLOCK3:
        case O_MOVABLE_BLOCK4:
            AlterFloorHeight(item, 1024);
            break;

        case O_ROLLING_BLOCK:
            AlterFloorHeight(item, 2048);
            break;
        }
    }
}

void AddRoomFlipItems(ROOM_INFO *r)
{
    for (int16_t item_num = r->item_number; item_num != NO_ITEM;
         item_num = Items[item_num].next_item) {
        ITEM_INFO *item = &Items[item_num];

        switch (item->object_number) {
        case O_MOVABLE_BLOCK:
        case O_MOVABLE_BLOCK2:
        case O_MOVABLE_BLOCK3:
        case O_MOVABLE_BLOCK4:
            AlterFloorHeight(item, -1024);
            break;

        case O_ROLLING_BLOCK:
            AlterFloorHeight(item, -2048);
            break;
        }
    }
}

void TriggerCDTrack(int16_t value, int16_t flags, int16_t type)
{
    if (value <= 1 || value >= MAX_CD_TRACKS) {
        return;
    }

    switch (value) {
    case 28:
        if ((CDFlags[value] & IF_ONESHOT)
            && LaraItem->current_anim_state == AS_UPJUMP) {
            value = 29;
        }
        break;

    case 37:
        if (LaraItem->current_anim_state != AS_HANG) {
            return;
        }
        break;

    case 41:
        if (LaraItem->current_anim_state != AS_HANG) {
            return;
        }
        break;

    case 42:
        if ((CDFlags[value] & IF_ONESHOT)
            && LaraItem->current_anim_state == AS_HANG) {
            value = 43;
        }
        break;

    case 49:
        if (LaraItem->current_anim_state != AS_SURFTREAD) {
            return;
        }
        break;

    case 50:
        if (CDFlags[value] & IF_ONESHOT) {
            static int16_t gym_completion_counter = 0;
            gym_completion_counter++;
            if (gym_completion_counter == FRAMES_PER_SECOND * 4) {
                LevelComplete = 1;
                gym_completion_counter = 0;
            }
        } else if (LaraItem->current_anim_state != AS_WATEROUT) {
            return;
        }
        break;
    }

    TriggerNormalCDTrack(value, flags, type);
}

void TriggerNormalCDTrack(int16_t value, int16_t flags, int16_t type)
{
    if (CDFlags[value] & IF_ONESHOT) {
        return;
    }

    if (type == TT_SWITCH) {
        CDFlags[value] ^= flags & IF_CODE_BITS;
    } else if (type == TT_ANTIPAD) {
        CDFlags[value] &= -1 - (flags & IF_CODE_BITS);
    } else if (flags & IF_CODE_BITS) {
        CDFlags[value] |= flags & IF_CODE_BITS;
    }

    if ((CDFlags[value] & IF_CODE_BITS) == IF_CODE_BITS) {
        if (flags & IF_ONESHOT) {
            CDFlags[value] |= IF_ONESHOT;
        }
        if (value != CDTrack) {
            S_MusicPlay(value);
        }
    } else {
        S_MusicStop();
    }
}

int GetSecretCount()
{
    int count = 0;
    uint32_t secrets = 0;

    for (int i = 0; i < RoomCount; i++) {
        ROOM_INFO *r = &RoomInfo[i];
        FLOOR_INFO *floor = &r->floor[0];
        for (int j = 0; j < r->y_size * r->x_size; j++, floor++) {
            int k = floor->index;
            if (!k) {
                continue;
            }

            while (1) {
                uint16_t floor = FloorData[k++];

                switch (floor & DATA_TYPE) {
                case FT_DOOR:
                case FT_ROOF:
                case FT_TILT:
                    k++;
                    break;

                case FT_LAVA:
                    break;

                case FT_TRIGGER: {
                    uint16_t trig_type = (floor & 0x3F00) >> 8;
                    k++; // skip basic trigger stuff

                    if (trig_type == TT_SWITCH || trig_type == TT_KEY
                        || trig_type == TT_PICKUP) {
                        k++;
                    }

                    while (1) {
                        int16_t command = FloorData[k++];
                        if (TRIG_BITS(command) == TO_CAMERA) {
                            k++;
                        } else if (TRIG_BITS(command) == TO_SECRET) {
                            int16_t number = command & VALUE_BITS;
                            if (!(secrets & (1 << number))) {
                                secrets |= (1 << number);
                                count++;
                            }
                        }

                        if (command & END_BIT) {
                            break;
                        }
                    }
                    break;
                }
                }

                if (floor & END_BIT) {
                    break;
                }
            }
        }
    }

    return count;
}

void T1MInjectGameControl()
{
    INJECT(0x004133B0, ControlPhase);
    INJECT(0x00413660, AnimateItem);
    INJECT(0x00413960, GetChange);
    INJECT(0x00413A10, TranslateItem);
    INJECT(0x00413A80, GetFloor);
    INJECT(0x00413C60, GetWaterHeight);
    INJECT(0x00413D60, GetHeight);
    INJECT(0x00413FA0, RefreshCamera);
    INJECT(0x00414080, TestTriggers);
    INJECT(0x00414820, TriggerActive);
    INJECT(0x00414880, GetCeiling);
    INJECT(0x00414AE0, GetDoor);
    INJECT(0x00414B30, LOS);
    INJECT(0x00414BD0, zLOS);
    INJECT(0x00414E50, xLOS);
    INJECT(0x004150C0, ClipTarget);
    INJECT(0x004151A0, FlipMap);
    INJECT(0x00415310, TriggerCDTrack);
    INJECT(0x00438920, CheckCheatMode);
}
