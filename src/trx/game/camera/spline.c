#include <trx/game/camera/spline.h>

#include <trx/game/camera.h>
#include <trx/game/lara.h>

void Spline_SetupData(
    SPLINE_DATA *const data, const int32_t slot_idx, const int32_t camera_idx)
{
    const FLYBY_CAMERA *const camera = Camera_GetFlybyCamera(camera_idx);
    data->pos.x[slot_idx] = camera->pos.x;
    data->pos.y[slot_idx] = camera->pos.y;
    data->pos.z[slot_idx] = camera->pos.z;

    if (camera->flags.focus_lara) {
        const ITEM *const lara_item = Lara_GetItem();
        data->target.x[slot_idx] = lara_item->pos.x;
        data->target.y[slot_idx] = lara_item->pos.y;
        data->target.z[slot_idx] = lara_item->pos.z;
    } else {
        data->target.x[slot_idx] = camera->target.x;
        data->target.y[slot_idx] = camera->target.y;
        data->target.z[slot_idx] = camera->target.z;
    }

    if (camera->flags.target_item) {
        const ITEM *const item = Item_Get(camera->timer);
        if (item != nullptr) {
            data->target.x[slot_idx] = item->pos.x;
            data->target.y[slot_idx] = item->pos.y;
            data->target.z[slot_idx] = item->pos.z;
        }
    }

    data->roll[slot_idx] = camera->roll;
    data->speed[slot_idx] = camera->speed;
    data->fov[slot_idx] = camera->fov;
}

int32_t Spline_Calculate(
    int32_t t, const int32_t *const knots, const int32_t knot_count)
{
    int32_t segment = t * (knot_count - 3) >> 16;
    if (segment >= knot_count - 3) {
        segment = knot_count - 4;
    }

    const int32_t *const control = &knots[segment];
    t = t * (knot_count - 3) - segment * SPLINE_ONE;

    const int32_t cubic = (control[1] >> 1) - (control[2] >> 1) - control[2]
        + control[1] + (control[3] >> 1) + ((-control[0] - 1) >> 1);
    const int32_t quadratic = 2 * control[2] - 2 * control[1]
        - (control[1] >> 1) - (control[3] >> 1) + control[0];

    const int64_t t64 = t;
    const int64_t poly = t64
        * ((t64 * ((t64 * cubic >> 16) + quadratic) >> 16) + (control[2] >> 1)
           + ((-control[0] - 1) >> 1));

    return (poly >> 16) + control[1];
}

int32_t Spline_GetNearestPosition(
    const XYZ_32 pos, const SPLINE_DATA *const data, const int32_t spline_count)
{
    int32_t best_pos = 0;
    int32_t sample_pos = 0;
    int32_t step = 0x2000;

    for (int32_t i = 0; i < 8; i++) {
        int32_t best_distance = INT32_MAX;
        for (int32_t j = 0; j < 8; j++) {
            const XYZ_32 test_pos = {
                .x = Spline_Calculate(sample_pos, data->pos.x, spline_count)
                    - pos.x,
                .y = Spline_Calculate(sample_pos, data->pos.y, spline_count)
                    - pos.y,
                .z = Spline_Calculate(sample_pos, data->pos.z, spline_count)
                    - pos.z,
            };
            const int32_t distance = XYZ_32_GetLength(test_pos);
            if (distance <= best_distance) {
                best_pos = sample_pos;
                best_distance = distance;
            }

            sample_pos += step;
            if (sample_pos > SPLINE_ONE) {
                break;
            }
        }

        step >>= 1;
        sample_pos = MAX(best_pos - (step << 1), 0);
    }

    CLAMP(best_pos, 0, SPLINE_ONE);
    return best_pos;
}
