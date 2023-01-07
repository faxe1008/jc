#include <string_builder.h>
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void builder_reset(StringBuilder_t* builder)
{
    assert(builder);
    memset(builder->buffer, 0, builder->capacity);
    builder->capacity = 0;
    builder->pos = 0;
}

bool builder_resize(StringBuilder_t* builder, size_t capacity)
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

static inline bool builder_ensure_capacity(StringBuilder_t* builder, size_t len)
{
    if (builder->pos + len < builder->capacity)
        return true;
    size_t additional_capacity = builder->capacity;
    if (builder->capacity < len) 
        additional_capacity = len;
    return builder_resize(builder, builder->capacity + additional_capacity);
}

bool builder_append_ch(StringBuilder_t* builder, char ch)
{
    assert(builder);
    if (!builder_ensure_capacity(builder, 1))
        return false;
    builder->buffer[builder->pos++] = ch;
    return true;
}

bool builder_append_chrs(StringBuilder_t* builder, char ch, size_t count)
{
    assert(builder);
    if (!builder_ensure_capacity(builder, count))
        return false;
    memset(&builder->buffer[builder->pos], ch, count);
    builder->pos += count;
    return true;
}

bool builder_append(StringBuilder_t* builder, const char* format, ...)
{
    assert(builder);
    if (builder->pos >= builder->capacity)
        return false;
    va_list args;
    va_start(args, format);
    size_t len = (size_t)vsnprintf(NULL, 0, format, args);
    va_end(args);
    va_start(args, format);
    if (!builder_ensure_capacity(builder, len)) 
        return false;
    vsnprintf(&builder->buffer[builder->pos], builder->capacity - builder->pos, format, args);
    builder->pos += len;
    va_end(args);
    return true;
}

void builder_append_escaped_str(StringBuilder_t* builder, const char* str)
{
    assert(builder);
    size_t str_len = strlen(str);
    if (!builder_ensure_capacity(builder, str_len))
        return;
    for (size_t i = 0; i < str_len; i++) {
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
