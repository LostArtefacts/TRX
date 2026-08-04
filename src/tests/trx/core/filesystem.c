#include <harness/harness.h>

#include <trx/core/filesystem.h>
#include <trx/core/memory.h>

static void M_CheckStem(const char *const path, const char *const expected)
{
    char *stem = File_GetStem(path);
    CHECK_EQ_STR(stem, expected);
    Memory_FreePointer(&stem);
}

TEST(a_base_name_is_what_follows_the_last_separator)
{
    CHECK_EQ_STR(File_GetBaseName("wall.tr2"), "wall.tr2");
    CHECK_EQ_STR(File_GetBaseName("data/wall.tr2"), "wall.tr2");
    CHECK_EQ_STR(File_GetBaseName("data\\wall.tr2"), "wall.tr2");
    CHECK_EQ_STR(File_GetBaseName("data\\levels/wall.tr2"), "wall.tr2");
    CHECK_NULL(File_GetBaseName(nullptr));
}

TEST(a_stem_is_the_file_name_without_its_extension)
{
    M_CheckStem("wall.tr2", "wall");
    M_CheckStem("wall", "wall");
}

TEST(a_stem_keeps_neither_directory_nor_separator_flavour)
{
    M_CheckStem("data/wall.tr2", "wall");
    M_CheckStem("data\\wall.tr2", "wall");
    M_CheckStem("data\\levels/wall.tr2", "wall");
}

// A name is cut at the last dot, so a version or a date in it stays part of
// the stem rather than ending it.
TEST(a_stem_ends_at_the_last_dot)
{
    M_CheckStem("wall.v2.tr2", "wall.v2");
    M_CheckStem(".hidden", "");
}

TEST(a_path_that_is_not_there_has_no_stem)
{
    CHECK_NULL(File_GetStem(nullptr));
}
