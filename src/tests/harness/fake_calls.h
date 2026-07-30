#pragma once

// What a fake was asked to do, recorded rather than performed.
//
// A fake records at the call site, naming the call and handing over the
// arguments it was given. Nothing declares the shape of the record: the key of
// an argument is the expression that supplied it and the type comes from that
// expression too, so a recorded call cannot disagree with the call that made
// it, and a new one needs no declaration anywhere.
//
// The Lua side reads them grouped by call - calls.play.count, calls.play.track
// - with the whole sequence in order alongside, for a test that cares which
// call came first.

#include <trx/core/value.h>

#include <lauxlib.h>

typedef struct {
    const char *name; // nullptr terminates an argument list
    TRX_VALUE value;
} FAKE_ARG;

// One argument of a recorded call. `count` and `name` belong to the log, so an
// argument spelled either way stops the run rather than answering in place of
// one.
#define FV(expr_) ((FAKE_ARG) { .name = #expr_, .value = Value_Of(expr_) })

// A string argument. A char array does not decay in _Generic, so it cannot go
// through FV; the recorder copies what this points at, since the buffer a fake
// holds it in is reused by the next call.
#define FV_STR(expr_)                                                          \
    ((FAKE_ARG) {                                                              \
        .name = #expr_,                                                        \
        .value = { .type = TVT_STRING, .as_str = (expr_) },                    \
    })

// Records one call: its name, then zero or more FV()/FV_STR() arguments. The
// __VA_OPT__ comma is what lets a call with no arguments through unchanged.
#define FAKE_RECORD(name_, ...)                                                \
    FakeCalls_Record(                                                          \
        name_,                                                                 \
        (const FAKE_ARG[]) { __VA_ARGS__ __VA_OPT__(, ) { .name = nullptr } })

void FakeCalls_Record(const char *name, const FAKE_ARG *args);

// Clears the log and runs every reset a fake registered. The surface runner
// calls this between cases, so each one starts from the same world.
void FakeCalls_Reset(void);

// A fake holds state beyond the log - a track marked as playing, a stream slot
// - and says here what to clear. Registering it means no test has to wire it
// up.
void FakeCalls_OnReset(void (*func)(void));

#define FAKE_ON_RESET(func_)                                                   \
    __attribute__((constructor)) static void M_Register_##func_(void)          \
    {                                                                          \
        FakeCalls_OnReset(func_);                                              \
    }

// Pushes the log as one table: each call name holds `count` and the arguments
// of the most recent one, and 1..n hold the calls in the order they arrived. A
// name that was never called reads as a count of zero rather than nil, so a
// test can assert that something did not happen.
int FakeCalls_Push(lua_State *L);
