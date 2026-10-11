// Wraps text a word at a time against the test's own UI_Text_Measure, which is
// enough to see where a line ends without the font.

#include <trx/core/memory.h>
#include <trx/game/ui/text.h>

#include <string.h>

char *UI_Text_WordWrap(
    const char *const text, const float scale, const float max_width)
{
    if (text == nullptr || max_width <= 0) {
        return nullptr;
    }
    const size_t len = strlen(text);
    char *const result = Memory_Alloc(len + 1);
    char *const line = Memory_Alloc(len + 1);
    size_t out = 0;
    size_t line_len = 0;
    size_t i = 0;
    while (i <= len) {
        const size_t word_start = i;
        while (i < len && text[i] != ' ' && text[i] != '\n') {
            i++;
        }
        const size_t word_len = i - word_start;
        memcpy(line + line_len, text + word_start, word_len);
        line[line_len + word_len] = '\0';
        float w = 0.0f;
        float h = 0.0f;
        UI_Text_Measure(line, &w, &h, (UI_TEXT_SETTINGS) { .scale = scale });
        if (w > max_width && line_len > 0) {
            result[out - 1] = '\n';
            memcpy(line, text + word_start, word_len);
            line_len = 0;
        }
        memcpy(result + out, text + word_start, word_len);
        out += word_len;
        line_len += word_len;
        if (i < len) {
            result[out++] = text[i];
            if (text[i] == '\n') {
                line_len = 0;
            } else {
                line[line_len++] = ' ';
            }
        }
        i++;
    }
    result[out] = '\0';
    Memory_Free(line);
    return result;
}
