// Copying a parsed JSON value.
//
// The settings file keeps what it read so it can put back the keys the writer
// did not produce - another game's declared options. That only works if the
// copy owns everything it points at: the file is re-read on every launch, and
// the value it hands over outlives the tree it came from.

#include <harness/harness.h>

#include <trx/core/json.h>
#include <trx/core/memory.h>

#include <string.h>

static JSON_VALUE *M_Parse(const char *const text)
{
    return JSON_Parse(text, strlen(text));
}

// Copies, frees the original, and only then reads - so anything the copy still
// shared with it would be read after the free.
static char *M_CopyAndWrite(const char *const text)
{
    JSON_VALUE *const parsed = M_Parse(text);
    CHECK_NOT_NULL(parsed);
    JSON_VALUE *const copy = JSON_ValueCopy(parsed);
    JSON_ValueFree(parsed);

    size_t size = 0;
    char *const written = JSON_WriteMinified(copy, &size);
    JSON_ValueFree(copy);
    return written;
}

TEST(a_copy_holds_what_the_original_held)
{
    char *const written = M_CopyAndWrite(
        "{\"a\":1,\"b\":\"text\",\"c\":true,\"d\":false,\"e\":null,"
        "\"f\":[1,2,3],\"g\":{\"h\":\"nested\"}}");
    CHECK_NOT_NULL(written);
    CHECK_EQ_STR(
        written,
        "{\"a\":1,\"b\":\"text\",\"c\":true,\"d\":false,\"e\":null,"
        "\"f\":[1,2,3],\"g\":{\"h\":\"nested\"}}");
    Memory_Free(written);
}

TEST(a_number_is_copied_as_it_was_written)
{
    // A number is stored as its text. Copying it through a double would round
    // a setting the player never touched.
    char *const written = M_CopyAndWrite("{\"a\":1.500,\"b\":100000000000}");
    CHECK_NOT_NULL(written);
    CHECK_EQ_STR(written, "{\"a\":1.500,\"b\":100000000000}");
    Memory_Free(written);
}

TEST(freeing_a_copy_leaves_the_original_whole)
{
    JSON_VALUE *const parsed = M_Parse("{\"a\":[1,{\"b\":\"text\"}]}");
    CHECK_NOT_NULL(parsed);
    JSON_ValueFree(JSON_ValueCopy(parsed));

    size_t size = 0;
    char *const written = JSON_WriteMinified(parsed, &size);
    JSON_ValueFree(parsed);
    CHECK_NOT_NULL(written);
    CHECK_EQ_STR(written, "{\"a\":[1,{\"b\":\"text\"}]}");
    Memory_Free(written);
}

TEST(copying_nothing_gives_nothing)
{
    CHECK(JSON_ValueCopy(nullptr) == nullptr);
}
