#include <trx/game/savegame/file_write_io.h>

#include <trx/debug.h>
#include <trx/json.h>
#include <trx/memory.h>

#define M_MAX_STACK_SIZE 10

typedef struct SG_WRITE_IO {
    JSON_VALUE *stack[M_MAX_STACK_SIZE];
    JSON_VALUE *current;
    size_t current_pos;
} SG_WRITE_IO;

static void M_PushValue(SG_WRITE_IO *const io, JSON_VALUE *const value)
{
    io->current_pos++;
    io->stack[io->current_pos] = value;
    io->current = value;
}

static void M_Pop(SG_WRITE_IO *const io)
{
    io->current_pos--;
    io->current = io->stack[io->current_pos];
}

void SG_WriteIO_PushObject(SG_WRITE_IO *const io)
{
    JSON_OBJECT *const child = JSON_ObjectNew();
    M_PushValue(io, JSON_ValueFromObject(child));
}

void SG_WriteIO_PushArray(SG_WRITE_IO *const io)
{
    JSON_ARRAY *const child = JSON_ArrayNew();
    M_PushValue(io, JSON_ValueFromArray(child));
}

void SG_WriteIO_PopAndSet(SG_WRITE_IO *const io, const char *const key)
{
    JSON_OBJECT *const parent =
        JSON_ValueAsObject(io->stack[io->current_pos - 1]);
    ASSERT(parent != nullptr);
    JSON_ObjectAppend(parent, key, io->current);
    M_Pop(io);
}

void SG_WriteIO_PopAndAppend(SG_WRITE_IO *const io)
{
    JSON_ARRAY *const parent =
        JSON_ValueAsArray(io->stack[io->current_pos - 1]);
    ASSERT(parent != nullptr);
    JSON_ArrayAppend(parent, io->current);
    M_Pop(io);
}

JSON_OBJECT *SG_WriteIO_GetCurrentObject(SG_WRITE_IO *const io)
{
    return JSON_ValueAsObject(io->current);
}

void SG_WriteIO_PushString(SG_WRITE_IO *const io, const char *const value)
{
    M_PushValue(io, JSON_ValueFromString(value));
}

void SG_WriteIO_PushBool(SG_WRITE_IO *const io, const bool value)
{
    M_PushValue(io, JSON_ValueFromBool(value));
}

void SG_WriteIO_PushInt(SG_WRITE_IO *const io, const int32_t value)
{
    M_PushValue(io, JSON_ValueFromInt(value));
}

void SG_WriteIO_PushDouble(SG_WRITE_IO *const io, const double value)
{
    M_PushValue(io, JSON_ValueFromDouble(value));
}

SG_WRITE_IO *SG_WriteIO_Create(void)
{
    SG_WRITE_IO *const io = Memory_Alloc(sizeof(*io));
    JSON_OBJECT *const root_obj = JSON_ObjectNew();
    io->stack[0] = JSON_ValueFromObject(root_obj);
    io->current = io->stack[0];
    return io;
}

void SG_WriteIO_Destroy(SG_WRITE_IO *const io)
{
    JSON_ValueFree(io->stack[0]);
    Memory_Free(io);
}

JSON_VALUE *SG_WriteIO_GetRoot(SG_WRITE_IO *const io)
{
    ASSERT(io->stack[0] != nullptr);
    return io->stack[0];
}
