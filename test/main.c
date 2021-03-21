#include "test.h"

#include "test_config.h"
#include "test_gameflow.h"

size_t tests = 0;
size_t fails = 0;

int main(int argc, char *argv[])
{
    test_empty_config();
    test_config_override();
    test_gameflow_equality();
    TEST_RESULTS();
}
