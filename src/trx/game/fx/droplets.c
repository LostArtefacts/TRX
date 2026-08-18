// TR4 water droplets: drops that fall off Lara's meshes after she leaves
// water, drawn as short additive line streaks. Each Lara mesh carries a
// wetness value (see LARA_INFO.wet) that is pinned while the mesh is
// underwater and drained here as droplets spawn. Not to be confused with
// fx/water_particles.c, which is the TR3 residue drifting around Lara
// while she is submerged.
#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/lara.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

#include <string.h>

#define M_MAX_DROPLETS 32
#define M_WET_MAX 252
#define M_WET_DECAY 4
#define M_FADE_LIFE 16
#define M_SPAWN_INTERVAL 4

typedef struct {
    bool on;
    XYZ_32 pos;
    XYZ_32 prev_pos;
    RGB_888 color;
    int16_t y_vel;
    uint8_t gravity;
    uint8_t life;
    int16_t room_num;
} M_DROPLET;

static M_DROPLET m_Droplets[M_MAX_DROPLETS];
static int32_t m_NextDroplet = 0;
static int32_t m_Ticks = 0;
static bool m_MeshUnderwater[LM_NUMBER_OF];

static bool M_IsEnabled(void)
{
    return g_Config.visuals.enable_droplets;
}

static M_DROPLET *M_GetFree(void)
{
    // Prefer a free slot; failing that, steal the shortest-lived droplet.
    M_DROPLET *best = nullptr;
    int32_t best_idx = 0;
    int32_t min_life = 0x7FFFFFFF;
    for (int32_t i = 0; i < M_MAX_DROPLETS; i++) {
        const int32_t idx = (m_NextDroplet + i) % M_MAX_DROPLETS;
        M_DROPLET *const droplet = &m_Droplets[idx];
        if (!droplet->on) {
            m_NextDroplet = (idx + 1) % M_MAX_DROPLETS;
            return droplet;
        }
        if (droplet->life < min_life) {
            min_life = droplet->life;
            best = droplet;
            best_idx = idx;
        }
    }
    m_NextDroplet = (best_idx + 1) % M_MAX_DROPLETS;
    return best;
}

static void M_UpdateWetness(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item == nullptr || !lara->mesh_pos_matrices_valid) {
        return;
    }

    for (LARA_MESH mesh = LM_FIRST; mesh < LM_NUMBER_OF; mesh++) {
        XYZ_32 pos = {};
        if (!Lara_GetMeshPos(mesh, &pos)) {
            return;
        }
        // Sample the torso and head closer to the mesh center.
        if (mesh == LM_TORSO) {
            pos.y -= 120;
        } else if (mesh == LM_HEAD) {
            pos.y -= 60;
        }

        int16_t room_num = lara_item->room_num;
        Room_GetSector(pos, &room_num);
        m_MeshUnderwater[mesh] = Room_Get(room_num)->flags.underwater;
        if (m_MeshUnderwater[mesh]) {
            lara->wet[mesh] = M_WET_MAX;
        }
    }
}

static void M_TriggerLaraDrips(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item == nullptr || !lara->mesh_pos_matrices_valid) {
        return;
    }

    for (LARA_MESH mesh = LM_NUMBER_OF - 1; mesh > LM_HIPS; mesh--) {
        if (lara->wet[mesh] == 0 || m_MeshUnderwater[mesh]
            || (Random_GetDraw() & 0x1FF) >= lara->wet[mesh]) {
            continue;
        }

        XYZ_32 pos = {
            .x = (Random_GetDraw() & 0x1F) - 16,
            .y = (Random_GetDraw() & 0xF) + 16,
            .z = (Random_GetDraw() & 0x1F) - 16,
        };
        if (!Lara_GetMeshPos(mesh, &pos)) {
            return;
        }

        M_DROPLET *const droplet = M_GetFree();
        droplet->on = true;
        droplet->pos = pos;
        droplet->prev_pos = pos;
        droplet->color = (RGB_888) {
            .r = (Random_GetDraw() & 7) + 16,
            .g = (Random_GetDraw() & 7) + 24,
            .b = (Random_GetDraw() & 7) + 32,
        };
        droplet->y_vel = (Random_GetDraw() & 0x1F) + 32;
        droplet->gravity = (Random_GetDraw() & 0x1F) + 32;
        droplet->life = (Random_GetDraw() & 0x1F) + 16;
        droplet->room_num = lara_item->room_num;
        lara->wet[mesh] -= M_WET_DECAY;
    }
}

static void M_UpdateDroplet(M_DROPLET *const droplet)
{
    droplet->life--;
    if (droplet->life == 0) {
        droplet->on = false;
        return;
    }

    if (droplet->life < M_FADE_LIFE) {
        droplet->color.r -= droplet->color.r >> 3;
        droplet->color.g -= droplet->color.g >> 3;
        droplet->color.b -= droplet->color.b >> 3;
    }

    droplet->prev_pos = droplet->pos;
    droplet->y_vel += droplet->gravity;
    if (Room_Get(droplet->room_num)->flags.wind) {
        const XZ_32 wind = Sparks_GetSmokeWind();
        droplet->pos.x += wind.x >> 1;
        droplet->pos.z += wind.z >> 1;
    }
    droplet->pos.y += droplet->y_vel >> 5;

    int16_t room_num = droplet->room_num;
    const SECTOR *const sector = Room_GetSector(droplet->pos, &room_num);
    droplet->room_num = room_num;

    const int32_t height = Room_GetHeight(sector, droplet->pos);
    if (Room_Get(room_num)->flags.underwater || droplet->pos.y > height) {
        droplet->on = false;
    }
}

static void M_Reset(void)
{
    memset(m_Droplets, 0, sizeof(m_Droplets));
    memset(m_MeshUnderwater, 0, sizeof(m_MeshUnderwater));
    m_NextDroplet = 0;
    m_Ticks = 0;
}

static void M_Control(void)
{
    if (!M_IsEnabled()) {
        return;
    }

    M_UpdateWetness();
    m_Ticks = (m_Ticks + 1) % M_SPAWN_INTERVAL;
    if (m_Ticks == 0) {
        M_TriggerLaraDrips();
    }

    for (int32_t i = 0; i < M_MAX_DROPLETS; i++) {
        M_DROPLET *const droplet = &m_Droplets[i];
        if (droplet->on) {
            M_UpdateDroplet(droplet);
        }
    }
}

static void M_Draw(void)
{
    if (!M_IsEnabled()) {
        return;
    }

    const double ratio = Interpolation_GetWorldRate();
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;

    for (int32_t i = 0; i < M_MAX_DROPLETS; i++) {
        const M_DROPLET *const droplet = &m_Droplets[i];
        if (!droplet->on) {
            continue;
        }

        const XYZ_32 head = do_interp
            ? (XYZ_32) {
                  .x = (int32_t)LERP(droplet->prev_pos.x, droplet->pos.x, ratio),
                  .y = (int32_t)LERP(droplet->prev_pos.y, droplet->pos.y, ratio),
                  .z = (int32_t)LERP(droplet->prev_pos.z, droplet->pos.z, ratio),
              }
            : droplet->pos;

        // The streak trails one step behind the drop.
        XYZ_32 tail = head;
        tail.y -= droplet->y_vel >> 6;
        if (Room_Get(droplet->room_num)->flags.wind) {
            const XZ_32 wind = Sparks_GetSmokeWind();
            tail.x -= wind.x >> 1;
            tail.z -= wind.z >> 1;
        }

        const uint8_t r = droplet->color.r << 2;
        const uint8_t g = droplet->color.g << 2;
        const uint8_t b = droplet->color.b << 2;
        const RGBA_8888 head_color = { r, g, b, 255 };
        const RGBA_8888 tail_color = { r >> 1, g >> 1, b >> 1, 255 };
        OutputSource_PolyFX_StageLineSegment(
            tail, tail_color, head, head_color, 1.0f, DRAW_BLEND_ADD);
    }
}

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_DROPLETS; i++) {
        const M_DROPLET *const droplet = &m_Droplets[i];
        if (!droplet->on) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", droplet->pos);
        JSONW_WRITE(io, "color", droplet->color);
        JSONW_WRITE(io, "y_vel", droplet->y_vel);
        JSONW_WRITE(io, "gravity", droplet->gravity);
        JSONW_WRITE(io, "life", droplet->life);
        JSONW_WRITE(io, "room_num", droplet->room_num);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "drops");
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "drops")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "drops"));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= M_MAX_DROPLETS) {
            LOG_WARNING(
                "Malformed save: too many droplets. Extra droplets will be "
                "ignored.");
            break;
        }

        M_DROPLET *const droplet = &m_Droplets[i];
        MUST(JSON_PUSH_INDEX(io, i));
        MUST(JSON_READ(io, "pos", &droplet->pos));
        MUST(JSON_READ(io, "color", &droplet->color));
        MUST(JSON_READ(io, "y_vel", &droplet->y_vel));
        MUST(JSON_READ(io, "gravity", &droplet->gravity));
        MUST(JSON_READ(io, "life", &droplet->life));
        MUST(JSON_READ(io, "room_num", &droplet->room_num));
        MUST(JSON_POP(io));
        droplet->on = true;
        droplet->prev_pos = droplet->pos;
        m_NextDroplet = (i + 1) % M_MAX_DROPLETS;
    }

    MUST(JSON_POP(io));
    return OK;
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = M_Draw,
    .reset_func = M_Reset,
    .save_key = "droplets",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
