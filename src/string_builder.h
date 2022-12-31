#ifndef JSONC_STRING_BUILDER__
#define JSONC_STRING_BUILDER__

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char* buffer;
    size_t capacity;
    size_t pos;
} StringBuilder_t;

static void builder_reset(StringBuilder_t* builder)
{
    assert(builder);
    memset(builder->buffer, 0, builder->capacity);
    builder->capacity = 0;
    builder->pos = 0;
}

static bool builder_resize(StringBuilder_t* builder, size_t capacity)
{
    assert(builder);
    char* new_buffer = calloc(1, capacity);
    if (!new_buffer)
        return false;
    if (builder->buffer) {
        memcpy(new_buffer, builder->buffer, builder->capacity);
        free(builder->buffer);
    }
    builder->buffer = new_buffer;
    builder->capacity = capacity;
    return true;
}

static bool builder_append_ch(StringBuilder_t* builder, char ch)
{
    assert(builder);
    if (builder->pos + 1 >= builder->capacity) {
        if (!builder_resize(builder, builder->capacity + 16))
            return false;
    }
    builder->buffer[builder->pos++] = ch;
    return true;
}

static bool builder_append(StringBuilder_t* builder, const char* format, ...)
{
    assert(builder);
    if (builder->pos >= builder->capacity)
        return false;
    va_list args;
    va_start(args, format);
    size_t len = (size_t)vsnprintf(NULL, 0, format, args);
    va_end(args);
    va_start(args, format);
    if (builder->pos + len >= builder->capacity) {
        if (!builder_resize(builder, builder->capacity + len * 2))
            return false;
    }
    vsnprintf(&builder->buffer[builder->pos], builder->capacity - builder->pos, format, args);
    builder->pos += len;
    va_end(args);
    return true;
}

static void builder_append_escaped_str(StringBuilder_t* builder, const char* str)
{
    assert(builder);
    // TODO: unefficient because multiple realloc of the builder
    for (size_t i = 0; i < strlen(str); i++) {
        char ch = str[i];
        switch (ch) {
        case '\b':
            builder_append(builder, "\\b");
            break;
        case '\n':
            builder_append(builder, "\\n");
            break;
        case '\t':
            builder_append(builder, "\\t");
            break;
        case '\"':
            builder_append(builder, "\\\"");
            break;
        case '\\':
            builder_append(builder, "\\\\");
            break;
        default:
            builder_append_ch(builder, ch);
        }
    }
}

#endif