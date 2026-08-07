#pragma once

// The lifecycle of a module that stands itself up and needs no other module to
// have been brought up first.
//
// A module that registers here comes up when a session starts and goes down
// when it ends, so what it holds lasts a session. That matters because a mod
// switch restarts the game in place: state left from the last mod is state the
// next one never asked for. A load-time constructor gives the module the
// lifetime of the process instead.
//
// A module that does need another one up first is stood up by the shell by
// name, where the order can be read.

typedef void (*SUBSYSTEM_FUNC)(void);

// A tier comes up before the tiers after it and goes down after them. Within a
// tier the order is the linker's, which is the order the source list happens to
// be in, so nothing may rest on it.
typedef enum {
    // Memory, lookups and reporting that the tiers above hold on to.
    SUBSYSTEM_TIER_BASE,
    SUBSYSTEM_TIER_MAIN,
    SUBSYSTEM_TIER_NUMBER_OF,
} SUBSYSTEM_TIER;

// A module names only the phases it has.
typedef struct SUBSYSTEM {
    SUBSYSTEM_TIER tier;
    // Stand the module up. Only the tiers below this one are up.
    SUBSYSTEM_FUNC init;
    // Read what the module keeps on disk. The mod's paths are resolved by now.
    SUBSYSTEM_FUNC load;
    // Take the config as first read. This is the work the module already does
    // when one of its options is written later.
    SUBSYSTEM_FUNC apply_config;
    // Give back what the module holds. One session can ask twice, so a second
    // call has to be a no-op rather than a second free.
    SUBSYSTEM_FUNC shutdown;
    // The registry's, filled in by Subsystem_Register.
    struct SUBSYSTEM *prev;
    struct SUBSYSTEM *next;
} SUBSYSTEM;

// The registry links the struct itself rather than copying it, so what is
// passed here lives as long as the process.
void Subsystem_Register(SUBSYSTEM *subsystem);

// Bring every registered subsystem up, a tier at a time.
void Subsystem_InitAll(void);

// Read what each subsystem keeps on disk, once the mod's paths are resolved.
void Subsystem_LoadAll(void);

// Hand each subsystem the config the session starts with.
void Subsystem_ApplyConfigAll(void);

// Take them down again, in the reverse of the order they came up in.
void Subsystem_ShutdownAll(void);

#define M_REGISTER_SUBSYSTEM(tier_, ...)                                       \
    __attribute__((constructor)) static void M_RegisterSubsystem(void)         \
    {                                                                          \
        static SUBSYSTEM m_Subsystem = { .tier = (tier_), __VA_ARGS__ };       \
        Subsystem_Register(&m_Subsystem);                                      \
    }

// One registration to a file: the symbol is the same every time, so a second
// one in the same file is a duplicate the compiler names. A module spread over
// several files can register from more than one of them, and each registration
// is its own subsystem.
#define REGISTER_SUBSYSTEM(...)                                                \
    M_REGISTER_SUBSYSTEM(SUBSYSTEM_TIER_MAIN, __VA_ARGS__)

// As REGISTER_SUBSYSTEM, at the base tier.
#define REGISTER_BASE_SUBSYSTEM(...)                                           \
    M_REGISTER_SUBSYSTEM(SUBSYSTEM_TIER_BASE, __VA_ARGS__)
