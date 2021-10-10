#include "game/camera.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/cinema.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "util.h"
#include "game/text.h"

#include <stddef.h>

void InitialiseCamera()
{
    Camera.shift = LaraItem->pos.y - WALL_L;

    Camera.target.x = LaraItem->pos.x;
    Camera.target.y = Camera.shift;
    Camera.target.z = LaraItem->pos.z;
    Camera.target.room_number = LaraItem->room_number;

    Camera.pos.x = Camera.target.x;
    Camera.pos.y = Camera.target.y;
    Camera.pos.z = Camera.target.z - 100;
    Camera.pos.room_number = Camera.target.room_number;

    Camera.target_distance = WALL_L * 3 / 2;
    Camera.item = NULL;

    Camera.number_frames = 1;
    Camera.type = CAM_CHASE;
    Camera.flags = 0;
    Camera.bounce = 0;
    Camera.number = NO_CAMERA;

    CalculateCamera();
}

void MoveCamera(GAME_VECTOR *ideal, int32_t speed)
{
	//speed *= ANIM_SCALE;
    Camera.pos.x += (ideal->x - Camera.pos.x) / speed;
    Camera.pos.z += (ideal->z - Camera.pos.z) / speed;
    Camera.pos.y += (ideal->y - Camera.pos.y) / speed;
    Camera.pos.room_number = ideal->room_number;

    ChunkyFlag = 0;

    FLOOR_INFO *floor = GetFloor(
        Camera.pos.x, Camera.pos.y, Camera.pos.z, &Camera.pos.room_number);
    int32_t height = GetHeight(floor, Camera.pos.x, Camera.pos.y, Camera.pos.z)
        - GROUND_SHIFT;

    if (Camera.pos.y >= height && ideal->y >= height) {
        LOS(&Camera.target, &Camera.pos);
        floor = GetFloor(
            Camera.pos.x, Camera.pos.y, Camera.pos.z, &Camera.pos.room_number);
        height = GetHeight(floor, Camera.pos.x, Camera.pos.y, Camera.pos.z)
            - GROUND_SHIFT;
    }

    int32_t ceiling =
        GetCeiling(floor, Camera.pos.x, Camera.pos.y, Camera.pos.z)
        + GROUND_SHIFT;
    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (Camera.bounce) {
        if (Camera.bounce > 0) {
            Camera.pos.y += Camera.bounce;
            Camera.target.y += Camera.bounce;
            Camera.bounce = 0;
        } else {
            int32_t shake;
            shake = (GetRandomControl() - 0x4000) * Camera.bounce / 0x7FFF;
            Camera.pos.x += shake;
            Camera.target.y += shake;
            shake = (GetRandomControl() - 0x4000) * Camera.bounce / 0x7FFF;
            Camera.pos.y += shake;
            Camera.target.y += shake;
            shake = (GetRandomControl() - 0x4000) * Camera.bounce / 0x7FFF;
            Camera.pos.z += shake;
            Camera.target.z += shake;
            Camera.bounce += 5;
        }
    }

    if (Camera.pos.y > height) {
        Camera.shift = height - Camera.pos.y;
    } else if (Camera.pos.y < ceiling) {
        Camera.shift = ceiling - Camera.pos.y;
    } else {
        Camera.shift = 0;
    }

    GetFloor(
        Camera.pos.x, Camera.pos.y + Camera.shift, Camera.pos.z,
        &Camera.pos.room_number);

    phd_LookAt(
        Camera.pos.x, Camera.pos.y + Camera.shift, Camera.pos.z,
        Camera.target.x, Camera.target.y, Camera.target.z, 0);

    Camera.actual_angle = phd_atan(
        Camera.target.z - Camera.pos.z, Camera.target.x - Camera.pos.x);
}

void ClipCamera(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom)
{
    if ((right > left) != (target_x < left)) {
        *y = target_y + (*y - target_y) * (left - target_x) / (*x - target_x);
        *x = left;
    }

    if ((bottom > top && target_y > top && *y < top)
        || (bottom < top && target_y < top && (*y) > top)) {
        *x = target_x + (*x - target_x) * (top - target_y) / (*y - target_y);
        *y = top;
    }
}

void ShiftCamera(
    int32_t *x, int32_t *y, int32_t target_x, int32_t target_y, int32_t left,
    int32_t top, int32_t right, int32_t bottom)
{
    int32_t shift;

    int32_t TL_square = SQUARE(target_x - left) + SQUARE(target_y - top);
    int32_t BL_square = SQUARE(target_x - left) + SQUARE(target_y - bottom);
    int32_t TR_square = SQUARE(target_x - right) + SQUARE(target_y - top);

    if (Camera.target_square < TL_square) {
        *x = left;
        shift = Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *y = target_y + ((top < bottom) ? -shift : shift);
    } else if (TL_square > MIN_SQUARE) {
        *x = left;
        *y = top;
    } else if (Camera.target_square < BL_square) {
        *x = left;
        shift = Camera.target_square - SQUARE(target_x - left);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *y = target_y + ((top < bottom) ? shift : -shift);
    } else if (BL_square > MIN_SQUARE) {
        *x = left;
        *y = bottom;
    } else if (Camera.target_square < TR_square) {
        shift = Camera.target_square - SQUARE(target_y - top);
        if (shift < 0) {
            return;
        }

        shift = phd_sqrt(shift);
        *x = target_x + ((left < right) ? shift : -shift);
        *y = top;
    } else {
        *x = right;
        *y = top;
    }
}

int32_t BadPosition(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    FLOOR_INFO *floor = GetFloor(x, y, z, &room_num);
    if (y >= GetHeight(floor, x, y, z) || y <= GetCeiling(floor, x, y, z)) {
        return 1;
    }
    return 0;
}

void SmartShift(
    GAME_VECTOR *ideal,
    void (*shift)(
        int32_t *x, int32_t *y, int32_t target_x, int32_t target_y,
        int32_t left, int32_t top, int32_t right, int32_t bottom))
{
    LOS(&Camera.target, ideal);

    ROOM_INFO *r = &RoomInfo[Camera.target.room_number];
    int32_t x_floor = (Camera.target.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (Camera.target.x - r->x) >> WALL_SHIFT;

    int16_t item_box = r->floor[x_floor + y_floor * r->x_size].box;
    BOX_INFO *box = &Boxes[item_box];

    r = &RoomInfo[ideal->room_number];
    x_floor = (ideal->z - r->z) >> WALL_SHIFT;
    y_floor = (ideal->x - r->x) >> WALL_SHIFT;

    int16_t camera_box = r->floor[x_floor + y_floor * r->x_size].box;
    if (camera_box != NO_BOX
        && (ideal->z < box->left || ideal->z > box->right || ideal->x < box->top
            || ideal->x > box->bottom)) {
        box = &Boxes[camera_box];
    }

    int32_t left = box->left;
    int32_t right = box->right;
    int32_t top = box->top;
    int32_t bottom = box->bottom;

    int32_t test = (ideal->z - WALL_L) | (WALL_L - 1);
    int32_t bad_left =
        BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_left) {
        camera_box = r->floor[x_floor - 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].left < left) {
            left = Boxes[camera_box].left;
        }
    }

    test = (ideal->z + WALL_L) & (~(WALL_L - 1));
    int32_t bad_right =
        BadPosition(ideal->x, ideal->y, test, ideal->room_number);
    if (!bad_right) {
        camera_box = r->floor[x_floor + 1 + y_floor * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].right > right) {
            right = Boxes[camera_box].right;
        }
    }

    test = (ideal->x - WALL_L) | (WALL_L - 1);
    int32_t bad_top = BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_top) {
        camera_box = r->floor[x_floor + (y_floor - 1) * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].top < top) {
            top = Boxes[camera_box].top;
        }
    }

    test = (ideal->x + WALL_L) & (~(WALL_L - 1));
    int32_t bad_bottom =
        BadPosition(test, ideal->y, ideal->z, ideal->room_number);
    if (!bad_bottom) {
        camera_box = r->floor[x_floor + (y_floor + 1) * r->x_size].box;
        if (camera_box != NO_ITEM && Boxes[camera_box].bottom > bottom) {
            bottom = Boxes[camera_box].bottom;
        }
    }

    left += STEP_L;
    right -= STEP_L;
    top += STEP_L;
    bottom -= STEP_L;

    int32_t noclip = 1;
    if (ideal->z < left && bad_left) {
        noclip = 0;
        if (ideal->x < Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, left,
                top, right, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, left,
                bottom, right, top);
        }
    } else if (ideal->z > right && bad_right) {
        noclip = 0;
        if (ideal->x < Camera.target.x) {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, right,
                top, left, bottom);
        } else {
            shift(
                &ideal->z, &ideal->x, Camera.target.z, Camera.target.x, right,
                bottom, left, top);
        }
    }

    if (noclip) {
        if (ideal->x < top && bad_top) {
            noclip = 0;
            if (ideal->z < Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z, top,
                    left, bottom, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z, top,
                    right, bottom, left);
            }
        } else if (ideal->x > bottom && bad_bottom) {
            noclip = 0;
            if (ideal->z < Camera.target.z) {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z,
                    bottom, left, top, right);
            } else {
                shift(
                    &ideal->x, &ideal->z, Camera.target.x, Camera.target.z,
                    bottom, right, top, left);
            }
        }
    }

    if (!noclip) {
        GetFloor(ideal->x, ideal->y, ideal->z, &ideal->room_number);
    }
}

void ChaseCamera(ITEM_INFO *item)
{
    GAME_VECTOR ideal;

    Camera.target_elevation += item->pos.x_rot;
    if (Camera.target_elevation > MAX_ELEVATION) {
        Camera.target_elevation = MAX_ELEVATION;
    } else if (Camera.target_elevation < -MAX_ELEVATION) {
        Camera.target_elevation = -MAX_ELEVATION;
    }

    int32_t distance =
        Camera.target_distance * phd_cos(Camera.target_elevation) >> W2V_SHIFT;
    ideal.y = Camera.target.y
        + (Camera.target_distance * phd_sin(Camera.target_elevation)
           >> W2V_SHIFT);

    Camera.target_square = SQUARE(distance);

    PHD_ANGLE angle = item->pos.y_rot + Camera.target_angle;
    ideal.x = Camera.target.x - (distance * phd_sin(angle) >> W2V_SHIFT);
    ideal.z = Camera.target.z - (distance * phd_cos(angle) >> W2V_SHIFT);
    ideal.room_number = Camera.pos.room_number;

    SmartShift(&ideal, ShiftCamera);

    if (Camera.fixed_camera) {
        MoveCamera(&ideal, Camera.speed);
    } else {
        MoveCamera(&ideal, CHASE_SPEED);
    }
}

int32_t ShiftClamp(GAME_VECTOR *pos, int32_t clamp)
{
    int32_t x = pos->x;
    int32_t y = pos->y;
    int32_t z = pos->z;

    FLOOR_INFO *floor = GetFloor(x, y, z, &pos->room_number);

    BOX_INFO *box = &Boxes[floor->box];
    if (z < box->left + clamp
        && BadPosition(x, y, z - clamp, pos->room_number)) {
        pos->z = box->left + clamp;
    } else if (
        z > box->right - clamp
        && BadPosition(x, y, z + clamp, pos->room_number)) {
        pos->z = box->right - clamp;
    }

    if (x < box->top + clamp
        && BadPosition(x - clamp, y, z, pos->room_number)) {
        pos->x = box->top + clamp;
    } else if (
        x > box->bottom - clamp
        && BadPosition(x + clamp, y, z, pos->room_number)) {
        pos->x = box->bottom - clamp;
    }

    int32_t height = GetHeight(floor, x, y, z) - clamp;
    int32_t ceiling = GetCeiling(floor, x, y, z) + clamp;

    if (height < ceiling) {
        ceiling = (height + ceiling) >> 1;
        height = ceiling;
    }

    if (y > height) {
        return height - y;
    } else if (pos->y < ceiling) {
        return ceiling - y;
    } else {
        return 0;
    }
}

void CombatCamera(ITEM_INFO *item)
{
    GAME_VECTOR ideal;

    Camera.target.z = item->pos.z;
    Camera.target.x = item->pos.x;

    if (Lara.target) {
        Camera.target_angle = item->pos.y_rot + Lara.target_angles[0];
        Camera.target_elevation = item->pos.x_rot + Lara.target_angles[1];
    } else {
        Camera.target_angle =
            item->pos.y_rot + Lara.torso_y_rot + Lara.head_y_rot;
        Camera.target_elevation =
            item->pos.x_rot + Lara.torso_x_rot + Lara.head_x_rot;
    }

    Camera.target_distance = COMBAT_DISTANCE;

    int32_t distance =
        Camera.target_distance * phd_cos(Camera.target_elevation) >> W2V_SHIFT;

    ideal.x = Camera.target.x
        - (distance * phd_sin(Camera.target_angle) >> W2V_SHIFT);
    ideal.y = Camera.target.y
        + (Camera.target_distance * phd_sin(Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = Camera.target.z
        - (distance * phd_cos(Camera.target_angle) >> W2V_SHIFT);
    ideal.room_number = Camera.pos.room_number;

    SmartShift(&ideal, ShiftCamera);
    MoveCamera(&ideal, Camera.speed);
}

void LookCamera(ITEM_INFO *item)
{
    GAME_VECTOR old;
    GAME_VECTOR ideal;

    old.z = Camera.target.z;
    old.x = Camera.target.x;

    Camera.target.z = item->pos.z;
    Camera.target.x = item->pos.x;

    Camera.target_angle = item->pos.y_rot + Lara.torso_y_rot + Lara.head_y_rot;
    Camera.target_elevation =
        item->pos.x_rot + Lara.torso_x_rot + Lara.head_x_rot;
    Camera.target_distance = WALL_L * 3 / 2;

    int32_t distance =
        Camera.target_distance * phd_cos(Camera.target_elevation) >> W2V_SHIFT;

    Camera.shift = ((-STEP_L * 2)  * phd_sin(Camera.target_elevation))/ ANIM_SCALE >> W2V_SHIFT;
    Camera.target.z += Camera.shift * phd_cos(item->pos.y_rot) >> W2V_SHIFT;
    Camera.target.x += Camera.shift * phd_sin(item->pos.y_rot) >> W2V_SHIFT;

    if (BadPosition(
            Camera.target.x, Camera.target.y, Camera.target.z,
            Camera.target.room_number)) {
        Camera.target.x = item->pos.x;
        Camera.target.z = item->pos.z;
    }

    Camera.target.y += ShiftClamp(&Camera.target, STEP_L + 50);

    ideal.x = Camera.target.x
        - (distance * phd_sin(Camera.target_angle) >> W2V_SHIFT);
    ideal.y = Camera.target.y
        + (Camera.target_distance * phd_sin(Camera.target_elevation)
           >> W2V_SHIFT);
    ideal.z = Camera.target.z
        - (distance * phd_cos(Camera.target_angle) >> W2V_SHIFT);
    ideal.room_number = Camera.pos.room_number;

    SmartShift(&ideal, ClipCamera);

    Camera.target.z = old.z + (Camera.target.z - old.z) / (Camera.speed*ANIM_SCALE);
    Camera.target.x = old.x + (Camera.target.x - old.x) / (Camera.speed*ANIM_SCALE);

    MoveCamera(&ideal, Camera.speed);
}

void FixedCamera()
{
    GAME_VECTOR ideal;
    OBJECT_VECTOR *fixed;

    fixed = &Camera.fixed[Camera.number];
    ideal.x = fixed->x;
    ideal.y = fixed->y;
    ideal.z = fixed->z;
    ideal.room_number = fixed->data;

    if (!LOS(&Camera.target, &ideal)) {
        ShiftClamp(&ideal, STEP_L);
    }

    Camera.fixed_camera = 1;

    MoveCamera(&ideal, Camera.speed);

    if (Camera.timer) {
        Camera.timer--;
        if (!Camera.timer) {
            Camera.timer = -1;
        }
    }
}

TEXTSTRING* cameraText = NULL;

void CalculateCamera()
{
    if (RoomInfo[Camera.pos.room_number].flags & RF_UNDERWATER) {
        if (!Camera.underwater) {
            SoundEffect(SFX_UNDERWATER, NULL, SPM_ALWAYS);
            Camera.underwater = 1;
        }
    } else if (Camera.underwater) {
        StopSoundEffect(SFX_UNDERWATER, NULL);
        Camera.underwater = 0;
    }

    if (Camera.type == CAM_CINEMATIC) {
        InGameCinematicCamera();
        return;
    }

    if (Camera.flags != NO_CHUNKY) {
        ChunkyFlag = 1;
    }

    int32_t fixed_camera =
        Camera.item && (Camera.type == CAM_FIXED || Camera.type == CAM_HEAVY);
    ITEM_INFO *item = fixed_camera ? Camera.item : LaraItem;

    int16_t *bounds = GetBoundsAccurate(item);

    int32_t y = item->pos.y;
    if (!fixed_camera) {
        y += bounds[FRAME_BOUND_MAX_Y]
            + ((bounds[FRAME_BOUND_MIN_Y] - bounds[FRAME_BOUND_MAX_Y]) * 3
               >> 2);
    } else {
        y += (bounds[FRAME_BOUND_MIN_Y] + bounds[FRAME_BOUND_MAX_Y]) / 2;
    }

    if (Camera.item && !fixed_camera) {
        bounds = GetBoundsAccurate(Camera.item);
        int16_t shift = phd_sqrt(
            SQUARE(Camera.item->pos.z - item->pos.z)
            + SQUARE(Camera.item->pos.x - item->pos.x));
        int16_t angle = phd_atan(
                            Camera.item->pos.z - item->pos.z,
                            Camera.item->pos.x - item->pos.x)
            - item->pos.y_rot;
        int16_t tilt = phd_atan(
            shift,
            y
                - (Camera.item->pos.y
                   + (bounds[FRAME_BOUND_MIN_Y] + bounds[FRAME_BOUND_MAX_Y])
                       / 2));
        angle >>= 1;
        tilt >>= 1;

        if (angle > -MAX_HEAD_ROTATION && angle < MAX_HEAD_ROTATION
            && tilt > MIN_HEAD_TILT_CAM && tilt < MAX_HEAD_TILT_CAM) {
            int16_t change = angle - Lara.head_y_rot;
            if (change > HEAD_TURN) {
                Lara.head_y_rot += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                Lara.head_y_rot -= HEAD_TURN;
            } else {
                Lara.head_y_rot += change;
            }

            change = tilt - Lara.head_x_rot;
            if (change > HEAD_TURN) {
                Lara.head_x_rot += HEAD_TURN;
            } else if (change < -HEAD_TURN) {
                Lara.head_x_rot -= HEAD_TURN;
            } else {
                Lara.head_x_rot += change;
            }

            Lara.torso_y_rot = Lara.head_y_rot;
            Lara.torso_x_rot = Lara.head_x_rot;

            Camera.type = CAM_LOOK;
            Camera.item->looked_at = 1;
        }
    }

    if (Camera.type == CAM_LOOK || Camera.type == CAM_COMBAT) {
        y -= STEP_L;
        Camera.target.room_number = item->room_number;

        if (Camera.fixed_camera) {
            Camera.target.y = y;
            Camera.speed = 1;
        } else {
            Camera.target.y += (y - Camera.target.y) >> 2;
            Camera.speed = Camera.type == CAM_LOOK ? LOOK_SPEED : COMBAT_SPEED;
        }

        Camera.fixed_camera = 0;

        if (Camera.type == CAM_LOOK) {
            LookCamera(item);
        } else {
            CombatCamera(item);
        }
    } else {
        Camera.target.x = item->pos.x;
        Camera.target.z = item->pos.z;

        if (Camera.flags == FOLLOW_CENTRE) {
            int16_t shift =
                (bounds[FRAME_BOUND_MIN_Z] + bounds[FRAME_BOUND_MAX_Z]) / 2;
            Camera.target.z += phd_cos(item->pos.y_rot) * shift >> W2V_SHIFT;
            Camera.target.x += phd_sin(item->pos.y_rot) * shift >> W2V_SHIFT;
        }

        Camera.target.room_number = item->room_number;

        if (Camera.fixed_camera != fixed_camera) {
            Camera.target.y = y;
            Camera.fixed_camera = 1;
            Camera.speed = 1;
        } else {
            Camera.target.y += (y - Camera.target.y) / 4;
            Camera.fixed_camera = 0;
        }

        FLOOR_INFO *floor = GetFloor(
            Camera.target.x, Camera.target.y, Camera.target.z,
            &Camera.target.room_number);
        if (Camera.target.y > GetHeight(
                floor, Camera.target.x, Camera.target.y, Camera.target.z)) {
            ChunkyFlag = 0;
        }

        if (Camera.type == CAM_CHASE || Camera.flags == CHASE_OBJECT) {
            ChaseCamera(item);
        } else {
            FixedCamera();
        }
    }

    Camera.last = Camera.number;
    Camera.fixed_camera = fixed_camera;

    if (Camera.type != CAM_HEAVY || Camera.timer == -1) {
        Camera.type = CAM_CHASE;
        Camera.number = NO_CAMERA;
        Camera.last_item = Camera.item;
        Camera.item = NULL;
        Camera.target_angle = 0;
        Camera.target_elevation = 0;
        Camera.target_distance = WALL_L * 3 / 2;
        Camera.flags = 0;
    }

    ChunkyFlag = 0;
    
#if 0    
    const double scale = 0.8;
    const int32_t text_height = 17 * scale;
    const int32_t text_offset_x = 0;
    const int32_t screen_margin_h = -20;
    const int32_t screen_margin_v = 18;

    char ammostring[80] = "";
    //char speedString[80] = "";
    
    sprintf(ammostring,"%d ; %d,%d,%d : %d,%d,%d : lara %d,%d,%d",fixed_camera, Camera.pos.x, Camera.pos.y, Camera.pos.z, Camera.target.x, Camera.target.y, Camera.target.z, LaraItem->pos.x, LaraItem->pos.y, LaraItem->pos.z);
    //sprintf(speedString,"%d - %g",item->speed, lara_speed_F);
    
    if (cameraText) {
        T_ChangeText(cameraText, ammostring);
    } else {
        
        cameraText = T_Print(
            -screen_margin_h - text_offset_x, (text_height*3) + screen_margin_v,
            ammostring);
        T_SetScale(cameraText, PHD_ONE * scale, PHD_ONE * scale);
        //T_RightAlign(LaraText2, 1);
    }
#endif
}

void T1MInjectGameCamera()
{
    INJECT(0x0040F920, InitialiseCamera);
    INJECT(0x0040F9B0, MoveCamera);
    INJECT(0x0040FCA0, ClipCamera);
    INJECT(0x0040FD40, ShiftCamera);
    INJECT(0x0040FEA0, SmartShift);
    INJECT(0x00410410, ChaseCamera);
    INJECT(0x00410530, ShiftClamp);
    INJECT(0x004107B0, CombatCamera);
    INJECT(0x004108F0, LookCamera);
    INJECT(0x00410B40, CalculateCamera);
}
