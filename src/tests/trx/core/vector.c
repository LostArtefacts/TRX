#include <harness/harness.h>

#include <trx/core/vector.h>

// VECTOR is what the config option map, the item lists and the savegame code
// are all built on, and none of it was covered. The container is generic - it
// memcpy's item_size bytes around - so the interesting behaviour is all in the
// shifting and the regrowth, not in any one caller.

static VECTOR *M_Of(const int32_t *const values, const int32_t count)
{
    VECTOR *const vector = Vector_Create(sizeof(int32_t));
    for (int32_t i = 0; i < count; i++) {
        Vector_Add(vector, &values[i]);
    }
    return vector;
}

static int32_t M_At(const VECTOR *const vector, const int32_t index)
{
    return *(int32_t *)Vector_Get(vector, index);
}

TEST(adding_appends_in_order)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 20, 30 }, 3);
    CHECK_EQ_INT(vector->count, 3);
    CHECK_EQ_INT(M_At(vector, 0), 10);
    CHECK_EQ_INT(M_At(vector, 2), 30);
    Vector_Free(vector);
}

TEST(growing_past_the_initial_capacity_keeps_every_element)
{
    // The default capacity is 4, so this forces several reallocs. A regrowth
    // that lost or reordered elements would go unnoticed until some caller
    // read back the wrong item.
    VECTOR *const vector = Vector_Create(sizeof(int32_t));
    for (int32_t i = 0; i < 100; i++) {
        Vector_Add(vector, &i);
    }
    CHECK_EQ_INT(vector->count, 100);
    CHECK(vector->capacity >= 100);
    for (int32_t i = 0; i < 100; i++) {
        CHECK_EQ_INT(M_At(vector, i), i);
    }
    Vector_Free(vector);
}

TEST(inserting_shifts_the_later_elements_up)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 30 }, 2);
    Vector_Insert(vector, 1, &(int32_t) { 20 });
    CHECK_EQ_INT(vector->count, 3);
    CHECK_EQ_INT(M_At(vector, 0), 10);
    CHECK_EQ_INT(M_At(vector, 1), 20);
    CHECK_EQ_INT(M_At(vector, 2), 30);

    // Inserting at the count is the same as appending.
    Vector_Insert(vector, vector->count, &(int32_t) { 40 });
    CHECK_EQ_INT(M_At(vector, 3), 40);

    // ...and at 0 everything else moves up.
    Vector_Insert(vector, 0, &(int32_t) { 5 });
    CHECK_EQ_INT(M_At(vector, 0), 5);
    CHECK_EQ_INT(M_At(vector, 1), 10);
    CHECK_EQ_INT(vector->count, 5);
    Vector_Free(vector);
}

TEST(removing_shifts_the_later_elements_down)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 20, 30, 40 }, 4);
    Vector_RemoveAt(vector, 1);
    CHECK_EQ_INT(vector->count, 3);
    CHECK_EQ_INT(M_At(vector, 0), 10);
    CHECK_EQ_INT(M_At(vector, 1), 30);
    CHECK_EQ_INT(M_At(vector, 2), 40);

    // Removing the last element shifts nothing, which is the case an
    // off-by-one in the memmove length would get wrong.
    Vector_RemoveAt(vector, vector->count - 1);
    CHECK_EQ_INT(vector->count, 2);
    CHECK_EQ_INT(M_At(vector, 1), 30);
    Vector_Free(vector);
}

TEST(removing_by_value_removes_only_the_first_match)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 20, 10 }, 3);
    CHECK(Vector_Remove(vector, &(int32_t) { 10 }));
    CHECK_EQ_INT(vector->count, 2);
    CHECK_EQ_INT(M_At(vector, 0), 20);
    CHECK_EQ_INT(M_At(vector, 1), 10);

    // A value that is not there is not an error, and changes nothing.
    CHECK(!Vector_Remove(vector, &(int32_t) { 99 }));
    CHECK_EQ_INT(vector->count, 2);
    Vector_Free(vector);
}

TEST(index_of_finds_the_first_match_and_last_index_of_the_last)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 20, 10 }, 3);
    CHECK_EQ_INT(Vector_IndexOf(vector, &(int32_t) { 10 }), 0);
    CHECK_EQ_INT(Vector_LastIndexOf(vector, &(int32_t) { 10 }), 2);
    CHECK_EQ_INT(Vector_IndexOf(vector, &(int32_t) { 99 }), -1);
    CHECK(Vector_Contains(vector, &(int32_t) { 20 }));
    CHECK(!Vector_Contains(vector, &(int32_t) { 99 }));
    Vector_Free(vector);
}

TEST(swapping_and_reversing_move_the_right_elements)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 20, 30, 40 }, 4);
    Vector_Swap(vector, 0, 3);
    CHECK_EQ_INT(M_At(vector, 0), 40);
    CHECK_EQ_INT(M_At(vector, 3), 10);

    Vector_Reverse(vector);
    CHECK_EQ_INT(M_At(vector, 0), 10);
    CHECK_EQ_INT(M_At(vector, 1), 30);
    CHECK_EQ_INT(M_At(vector, 2), 20);
    CHECK_EQ_INT(M_At(vector, 3), 40);
    Vector_Free(vector);
}

TEST(reversing_an_odd_number_of_elements_keeps_the_middle_one_put)
{
    VECTOR *const vector = M_Of((int32_t[]) { 10, 20, 30 }, 3);
    Vector_Reverse(vector);
    CHECK_EQ_INT(M_At(vector, 0), 30);
    CHECK_EQ_INT(M_At(vector, 1), 20);
    CHECK_EQ_INT(M_At(vector, 2), 10);
    Vector_Free(vector);
}

TEST(clearing_drops_the_count_but_keeps_the_capacity)
{
    VECTOR *const vector = Vector_Create(sizeof(int32_t));
    for (int32_t i = 0; i < 50; i++) {
        Vector_Add(vector, &i);
    }
    const int32_t grown_capacity = vector->capacity;

    // Callers clear and refill a vector every frame - hence keeping the buffer.
    Vector_Clear(vector);
    CHECK_EQ_INT(vector->count, 0);
    CHECK_EQ_INT(vector->capacity, grown_capacity);

    // ClearRealloc is the one that hands the memory back.
    Vector_ClearRealloc(vector);
    CHECK_EQ_INT(vector->count, 0);
    CHECK(vector->capacity < grown_capacity);

    // Either way the vector is still usable.
    Vector_Add(vector, &(int32_t) { 7 });
    CHECK_EQ_INT(M_At(vector, 0), 7);
    Vector_Free(vector);
}

TEST(expanding_reserves_room_the_caller_can_write_into)
{
    VECTOR *const vector = Vector_Create(sizeof(int32_t));
    int32_t *const room = Vector_Expand(vector, 3);
    CHECK_EQ_INT(vector->count, 3);
    room[0] = 10;
    room[2] = 30;
    CHECK_EQ_INT(M_At(vector, 0), 10);
    CHECK_EQ_INT(M_At(vector, 2), 30);
    Vector_Free(vector);
}

TEST(a_vector_of_structs_moves_whole_items_not_bytes)
{
    // item_size is whatever the caller says, so a shift that used the wrong
    // stride would corrupt every element after the one removed.
    typedef struct {
        int32_t id;
        double weight;
    } M_ENTRY;

    VECTOR *const vector = Vector_Create(sizeof(M_ENTRY));
    for (int32_t i = 0; i < 5; i++) {
        Vector_Add(vector, &(M_ENTRY) { .id = i, .weight = i * 1.5 });
    }
    Vector_RemoveAt(vector, 0);

    CHECK_EQ_INT(vector->count, 4);
    for (int32_t i = 0; i < 4; i++) {
        const M_ENTRY *const entry = Vector_Get(vector, i);
        CHECK_EQ_INT(entry->id, i + 1);
        CHECK(entry->weight == (i + 1) * 1.5);
    }
    Vector_Free(vector);
}
