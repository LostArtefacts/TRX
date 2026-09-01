#include <trx/game/gun/routines.h>

#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>

#include <string.h>

typedef struct {
    const char *name;
    void *func;
} M_NAMED;

typedef enum {
    M_FIRE,
    M_FLASH,
    M_SOUND,
    M_READY_ANIM,
    M_SMOKE_SIZE,
    M_NUM_ROUTINE_KINDS,
} M_ROUTINE_KIND;

static GUN_KIND_ROUTINES m_Kinds[NUM_WEAPON_TYPES] = {};
static bool m_KindHeld[NUM_WEAPON_TYPES] = {};
static VECTOR *m_Named[M_NUM_ROUTINE_KINDS] = {};

__attribute__((destructor)) static void M_Shutdown(void)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_Named); i++) {
        if (m_Named[i] != nullptr) {
            Vector_Free(m_Named[i]);
            m_Named[i] = nullptr;
        }
    }
}

static void M_Add(
    const M_ROUTINE_KIND kind, const char *const name, void *const func)
{
    ASSERT(name != nullptr && func != nullptr);
    if (m_Named[kind] == nullptr) {
        m_Named[kind] = Vector_Create(sizeof(M_NAMED));
    }
    const M_NAMED named = { .name = name, .func = func };
    Vector_Add(m_Named[kind], &named);
}

static void *M_Get(const M_ROUTINE_KIND kind, const char *const name)
{
    if (name == nullptr) {
        return nullptr;
    }
    for (int32_t i = 0; m_Named[kind] != nullptr && i < m_Named[kind]->count;
         i++) {
        const M_NAMED *const named = Vector_Get(m_Named[kind], i);
        if (strcmp(named->name, name) == 0) {
            return named->func;
        }
    }
    return nullptr;
}

void Gun_Routines_AddKind(
    const WEAPON_TYPE type, const GUN_KIND_ROUTINES routines)
{
    ASSERT(type >= 0 && type < NUM_WEAPON_TYPES);
    ASSERT(!m_KindHeld[type]);
    m_Kinds[type] = routines;
    m_KindHeld[type] = true;
}

const GUN_KIND_ROUTINES *Gun_Routines_GetKind(const WEAPON_TYPE type)
{
    if (type < 0 || type >= NUM_WEAPON_TYPES || !m_KindHeld[type]) {
        return nullptr;
    }
    return &m_Kinds[type];
}

void Gun_Routines_AddFire(
    const char *const name,
    void (*const func)(LARA_GUN_TYPE gun_type, bool running))
{
    M_Add(M_FIRE, name, (void *)func);
}

void Gun_Routines_AddFlash(
    const char *const name, GUN_FLASH (*const func)(void))
{
    M_Add(M_FLASH, name, (void *)func);
}

void Gun_Routines_AddSound(const char *const name, void (*const func)(bool))
{
    M_Add(M_SOUND, name, (void *)func);
}

void Gun_Routines_AddReadyAnim(
    const char *const name, int16_t (*const func)(void))
{
    M_Add(M_READY_ANIM, name, (void *)func);
}

void Gun_Routines_AddSmokeSize(
    const char *const name, uint8_t (*const func)(void))
{
    M_Add(M_SMOKE_SIZE, name, (void *)func);
}

void (*Gun_Routines_GetFire(const char *const name))(LARA_GUN_TYPE, bool)
{
    return (void (*)(LARA_GUN_TYPE, bool))M_Get(M_FIRE, name);
}

GUN_FLASH (*Gun_Routines_GetFlash(const char *const name))(void)
{
    return (GUN_FLASH (*)(void))M_Get(M_FLASH, name);
}

void (*Gun_Routines_GetSound(const char *const name))(bool)
{
    return (void (*)(bool))M_Get(M_SOUND, name);
}

int16_t (*Gun_Routines_GetReadyAnim(const char *const name))(void)
{
    return (int16_t (*)(void))M_Get(M_READY_ANIM, name);
}

uint8_t (*Gun_Routines_GetSmokeSize(const char *const name))(void)
{
    return (uint8_t (*)(void))M_Get(M_SMOKE_SIZE, name);
}
