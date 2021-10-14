#include "test_gameflow.h"

#include "filesystem.h"
#include "json.h"
#include "test.h"

#include <stdlib.h>
#include <string.h>

static char *read_file_text(const char *path)
{
    MYFILE *fp = FileOpen(path, FILE_OPEN_READ);
    if (!fp) {
        return NULL;
    }
    int data_size = FileSize(fp);
    char *data = malloc(data_size + 1);
    FileRead(data, sizeof(char), data_size, fp);
    data[data_size] = '\0';
    FileClose(fp);
    return data;
}

static void
assert_json_keys_equal(struct json_object_s *obj1, struct json_object_s *obj2)
{
    ASSERT_ULONG_EQUAL(obj1->length, obj2->length);
    struct json_object_element_s *obj1_elem = obj1->start;
    struct json_object_element_s *obj2_elem = obj2->start;
    while (obj1_elem) {
        ASSERT_STR_EQUAL(obj1_elem->name->string, obj2_elem->name->string);
        obj1_elem = obj1_elem->next;
        obj2_elem = obj2_elem->next;
    }
}

void test_gameflow_equality()
{
    char *gf1_text = read_file_text("cfg/Tomb1Main_gameflow.json5");
    char *gf2_text = read_file_text("cfg/Tomb1Main_gameflow_ub.json5");
    ASSERT_OK(gf1_text != NULL);
    ASSERT_OK(gf2_text != NULL);

    struct json_object_s *gf1_root = json_value_as_object(json_parse_ex(
        gf1_text, strlen(gf1_text), json_parse_flags_allow_json5, NULL, NULL,
        NULL));
    struct json_object_s *gf2_root = json_value_as_object(json_parse_ex(
        gf2_text, strlen(gf2_text), json_parse_flags_allow_json5, NULL, NULL,
        NULL));
    ASSERT_OK(gf1_root != NULL);
    ASSERT_OK(gf2_root != NULL);
    assert_json_keys_equal(gf1_root, gf2_root);

    struct json_object_s *strings1 =
        json_object_get_object(gf1_root, "strings");
    struct json_object_s *strings2 =
        json_object_get_object(gf2_root, "strings");
    ASSERT_OK(strings1 != NULL);
    ASSERT_OK(strings2 != NULL);
    assert_json_keys_equal(strings1, strings2);
}
