#ifndef JC_STRING_BUILDER__
#define JC_STRING_BUILDER__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char* buffer;
    size_t capacity;
    size_t pos;
} StringBuilder_t;

void builder_reset(StringBuilder_t* builder);
bool builder_resize(StringBuilder_t* builder, size_t capacity);
bool builder_append_ch(StringBuilder_t* builder, char ch);
bool builder_append_chrs(StringBuilder_t* builder, char ch, size_t count);
bool builder_append(StringBuilder_t* builder, const char* format, ...);
void builder_append_escaped_str(StringBuilder_t* builder, const char* str);
bool builder_append_unicode(StringBuilder_t* builder, uint32_t code_point);

#endif
