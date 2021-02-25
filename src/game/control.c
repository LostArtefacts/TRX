#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/camera.h"
#include "game/control.h"
#include "game/demo.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/inv.h"
#include "game/lara.h"
#include "game/misc.h"
#include "game/savegame.h"
#include "game/vars.h"
#include "specific/input.h"
#include "specific/shed.h"
#include "specific/sndpc.h"
#include "util.h"

int32_t ControlPhase(int32_t nframes, int demo_mode)
{
    int32_t return_val = 0;
    if (nframes > MAX_FRAMES) {
        nframes = MAX_FRAMES;
    }
    FrameCount += AnimationRate * nframes;
    if (FrameCount < 0) {
        return 0;
    }

    for (; FrameCount >= 0; FrameCount -= 0x10000) {
        if (CDTrack > 0)
            S_CDLoop();

        CheckCheatMode();
        if (LevelComplete) {
            return 1;
        }

        S_UpdateInput();

        if (ResetFlag) {
            return GF_EXIT_TO_TITLE;
        }

        if (demo_mode) {
            if (KeyData->keys_held) {
                return 1;
            }
            GetDemoInput();
            if (Input == -1) {
                return 1;
            }
        }

        if (Lara.death_count > DEATH_WAIT
            || (Lara.death_count > DEATH_WAIT_MIN && Input)
            || OverlayFlag == 2) {
            if (demo_mode) {
                return 1;
            }
            if (OverlayFlag == 2) {
                OverlayFlag = 1;
                return_val = Display_Inventory(INV_DEATH_MODE);
                if (return_val) {
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
                if (return_val) {
                    if (InventoryExtraData[0] != 1) {
                        return return_val;
                    }
                    if (CurrentLevel == LV_GYM) {
                        return GF_STARTGAME | LV_FIRSTLEVEL;
                    }

                    CreateSaveGameInfo();
                    S_SaveGame(
                        &SaveGame[0], sizeof(SAVEGAME_INFO),
                        InventoryExtraData[1]);
                    WriteTombAtiSettings();
                }
            }
        }

        int16_t item_num = NextItemActive;
        while (item_num != NO_ITEM) {
            int nex = Items[item_num].next_active;
            if (Objects[Items[item_num].object_number].control)
                (*Objects[Items[item_num].object_number].control)(item_num);
            item_num = nex;
        }

        item_num = NextFxActive;
        while (item_num != NO_ITEM) {
            int nex = Effects[item_num].next_active;
            if (Objects[Effects[item_num].object_number].control)
                (*Objects[Effects[item_num].object_number].control)(item_num);
            item_num = nex;
        }

        LaraControl(0);
        CalculateCamera();
        SoundEffects();
        ++SaveGame[0].timer;
        --HealthBarTimer;

#ifdef T1M_FEAT_GAMEPLAY
        if (T1MConfig.disable_healing_between_levels) {
            int lara_found = 0;
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

        if (T1MConfig.healthbar_showing_mode == T1M_BSM_ALWAYS
            || (T1MConfig.healthbar_showing_mode == T1M_BSM_FLASHING && LaraItem
                && LaraItem->hit_points < (LARA_HITPOINTS * 20) / 100)) {
            HealthBarTimer = 1;
        }
#endif
    }
    return 0;
}

void AnimateItem(ITEM_INFO* item)
{
    item->touch_bits = 0;
    item->hit_status = 0;

    ANIM_STRUCT* anim = &Anims[item->anim_number];

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
            int16_t* command = &AnimCommands[anim->command_index];
            for (int i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    TranslateItem(item, command[0], command[1], command[2]);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    item->fall_speed = command[0];
                    item->speed = command[1];
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
        int16_t* command = &AnimCommands[anim->command_index];
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
        int32_t speed = anim->velocity;
        if (anim->acceleration) {
            speed +=
                anim->acceleration * (item->frame_number - anim->frame_base);
        }
        item->speed = speed >> 16;
    } else {
        item->fall_speed += (item->fall_speed < FASTFALL_SPEED) ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    }

    item->pos.x += (phd_sin(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
    item->pos.z += (phd_cos(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
}

int32_t GetChange(ITEM_INFO* item, ANIM_STRUCT* anim)
{
    if (item->current_anim_state == item->goal_anim_state) {
        return 0;
    }

    ANIM_CHANGE_STRUCT* change = &AnimChanges[anim->change_index];
    for (int i = 0; i < anim->number_changes; i++, change++) {
        if (change->goal_anim_state == item->goal_anim_state) {
            ANIM_RANGE_STRUCT* range = &AnimRanges[change->range_index];
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

void TranslateItem(ITEM_INFO* item, int32_t x, int32_t y, int32_t z)
{
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);

    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}

FLOOR_INFO* GetFloor(int32_t x, int32_t y, int32_t z, int16_t* room_num)
{
    int16_t data;
    FLOOR_INFO* floor;
    ROOM_INFO* r = &RoomInfo[*room_num];
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
    ROOM_INFO* r = &RoomInfo[room_num];
    int32_t x_floor = (z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (x - r->x) >> WALL_SHIFT;
    FLOOR_INFO* floor = &r->floor[x_floor + y_floor * r->x_size];

    if (r->flags & RF_UNDERWATER) {
        while (floor->sky_room != NO_ROOM) {
            r = &RoomInfo[floor->sky_room];
            if (!(r->flags & RF_UNDERWATER)) {
                break;
            }
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
        return floor->ceiling << 8;
    } else {
        while (floor->pit_room != NO_ROOM) {
            r = &RoomInfo[floor->pit_room];
            if (r->flags & RF_UNDERWATER) {
                return floor->floor << 8;
            }
            int32_t x_floor = (z - r->z) >> WALL_SHIFT;
            int32_t y_floor = (x - r->x) >> WALL_SHIFT;
            floor = &r->floor[x_floor + y_floor * r->x_size];
        }
    }

    return NO_HEIGHT;
}

int16_t GetHeight(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z)
{
    HeightType = HT_WALL;
    while (floor->pit_room != NO_ROOM) {
        ROOM_INFO* r = &RoomInfo[floor->pit_room];
        int32_t x_floor = (z - r->z) >> WALL_SHIFT;
        int32_t y_floor = (x - r->x) >> WALL_SHIFT;
        floor = &r->floor[x_floor + y_floor * r->x_size];
    }

    int16_t height = floor->floor << 8;

    TriggerIndex = NULL;

    if (!floor->index) {
        return height;
    }

    int16_t* data = &FloorData[floor->index];
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
                    ITEM_INFO* item = &Items[trigger & VALUE_BITS];
                    OBJECT_INFO* object = &Objects[item->object_number];
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

void RefreshCamera(int16_t type, int16_t* data)
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

int32_t TriggerActive(ITEM_INFO* item)
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

int16_t GetCeiling(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z)
{
    int16_t* data;
    int16_t type;
    int16_t trigger;

    FLOOR_INFO* f = floor;
    while (f->sky_room != NO_ROOM) {
        ROOM_INFO* r = &RoomInfo[f->sky_room];
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
        ROOM_INFO* r = &RoomInfo[floor->pit_room];
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
                    ITEM_INFO* item = &Items[trigger & VALUE_BITS];
                    OBJECT_INFO* object = &Objects[item->object_number];
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

int16_t GetDoor(FLOOR_INFO* floor)
{
    if (!floor->index) {
        return NO_ROOM;
    }

    int16_t* data = &FloorData[floor->index];
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

int32_t LOS(GAME_VECTOR* start, GAME_VECTOR* target)
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

    FLOOR_INFO* floor =
        GetFloor(target->x, target->y, target->z, &target->room_number);

    if (ClipTarget(start, target, floor) && los1 == 1 && los2 == 1) {
        return 1;
    }

    return 0;
}

int32_t zLOS(GAME_VECTOR* start, GAME_VECTOR* target)
{
    FLOOR_INFO* floor;

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

int32_t xLOS(GAME_VECTOR* start, GAME_VECTOR* target)
{
    FLOOR_INFO* floor;

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

int32_t ClipTarget(GAME_VECTOR* start, GAME_VECTOR* target, FLOOR_INFO* floor)
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
    INJECT(0x00414820, TriggerActive);
    INJECT(0x00414880, GetCeiling);
    INJECT(0x00414AE0, GetDoor);
    INJECT(0x00414B30, LOS);
    INJECT(0x00414BD0, zLOS);
    INJECT(0x00414E50, xLOS);
    INJECT(0x004150C0, ClipTarget);
}
