#include <ctype.h>
#include <jsonc.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

JsonDocument_t* jsonc_new_doc()
{
    return (JsonDocument_t*)calloc(1, sizeof(JsonDocument_t));
}
JsonObject_t* jsonc_new_obj()
{
    return (JsonObject_t*)calloc(1, sizeof(JsonObject_t));
}
JsonArray_t* jsonc_new_array()
{
    return (JsonArray_t*)calloc(1, sizeof(JsonArray_t));
}

JsonValue_t* jsonc_new_value(JsonValueType_t ty, void* data)
{
    if (!data && ty != NULL_LITERAL && ty != BOOLEAN)
        return NULL;

    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = ty;
    switch (ty) {
    case STRING:
        value->string = (char*)calloc(1, strlen((const char*)data));
        if (!value->string)
            return NULL;
        strcpy(value->string, (const char*)data);
        break;
    case NUMBER:
        value->number = *(const double*)data;
        break;
    case OBJECT:
        value->object = data;
        break;
    case ARRAY:
        value->array = data;
        break;
    case BOOLEAN:
        value->boolean = data != 0 ? true : false;
        break;
    case NULL_LITERAL:
        break;
    }
    return value;
}

void jsonc_free_doc(JsonDocument_t* doc)
{
    if (!doc)
        return;
    if (doc->object)
        jsonc_free_obj(doc->object);
    if (doc->array)
        jsonc_free_array(doc->array);
    free(doc);
}

void jsonc_free_obj(JsonObject_t* obj)
{
    if (!obj)
        return;
    JsonObjectEntry_t* current = obj->entry;
    while (current) {
        JsonObjectEntry_t* next = current->next;
        jsonc_free_obj_entry(current);
        current = next;
    }
    free(obj);
}

void jsonc_free_array(JsonArray_t* arr)
{
    if (!arr)
        return;
    JsonArrayEntry_t* current = arr->entry;
    while (current) {
        JsonArrayEntry_t* next = current->next;
        jsonc_free_array_entry(current);
        current = next;
    }
    free(arr);
}

void jsonc_free_value(JsonValue_t* value)
{
    if (!value)
        return;
    if (value->ty == STRING && value->string) {
        free(value->string);
        value->string = NULL;
    }
    if (value->ty == OBJECT && value->object) {
        jsonc_free_obj(value->object);
        value->object = NULL;
    }
    if (value->ty == ARRAY && value->array) {
        jsonc_free_array(value->array);
        value->array = NULL;
    }
    free(value);
}

void jsonc_free_array_entry(JsonArrayEntry_t* entry)
{
    if (!entry)
        return;
    if (entry->value) {
        jsonc_free_value(entry->value);
        entry->value = NULL;
    }
    free(entry);
}
void jsonc_free_obj_entry(JsonObjectEntry_t* entry)
{
    if (!entry)
        return;
    if (entry->value) {
        jsonc_free_value(entry->value);
        entry->value = NULL;
    }
    if (entry->key) {
        free(entry->key);
        entry->key = NULL;
    }
    free(entry);
}

void jsonc_doc_set_obj(JsonDocument_t* doc, JsonObject_t* obj)
{
    if (!doc || !obj)
        return;
    if (doc->object && doc->object != obj)
        jsonc_free_obj(doc->object);
    if (doc->array)
        jsonc_free_array(doc->array);
    doc->object = obj;
}

void jsonc_doc_set_array(JsonDocument_t* doc, JsonArray_t* arr)
{
    if (!doc || !arr)
        return;
    if (doc->object)
        jsonc_free_obj(doc->object);
    if (doc->array && doc->array != arr)
        jsonc_free_array(doc->array);
    doc->array = arr;
}

void jsonc_array_insert(JsonArray_t* arr, JsonValue_t* value)
{
    if (!arr || !value)
        return;
    JsonArrayEntry_t* new_entry = (JsonArrayEntry_t*)calloc(1, sizeof(JsonArrayEntry_t));
    if (!new_entry)
        return;
    new_entry->value = value;

    if (!arr->entry) {
        arr->entry = new_entry;
        return;
    }

    JsonArrayEntry_t* entry = arr->entry;
    for (;;) {
        if (!entry->next)
            break;
        entry = entry->next;
    }
    entry->next = new_entry;
}

static JsonObjectEntry_t* jsonc_new_object_entry(const char* key, JsonValue_t* value)
{
    JsonObjectEntry_t* new_entry = (JsonObjectEntry_t*)calloc(1, sizeof(JsonObjectEntry_t));
    if (!new_entry)
        return NULL;
    new_entry->key = (char*)malloc(strlen(key));
    strcpy(new_entry->key, key);
    new_entry->value = value;
    return new_entry;
}

void jsonc_obj_set(JsonObject_t* obj, const char* key, JsonValue_t* value)
{
    if (!obj || !key || !value)
        return;

    // First entry in object
    if (!obj->entry) {
        JsonObjectEntry_t* new_entry = jsonc_new_object_entry(key, value);
        if (!new_entry)
            return;
        obj->entry = new_entry;
        return;
    }

    // Iterate through entries, check if key exists, only swap value if needed
    JsonObjectEntry_t* entry = obj->entry;
    for (;;) {
        if (entry->value != value && strcmp(entry->key, key) == 0) {
            if (entry->value)
                jsonc_free_value(entry->value);
            entry->value = value;
            return;
        }
        if (!entry->next)
            break;
        entry = entry->next;
    }

    // Key was not found, reached the end of the list
    JsonObjectEntry_t* new_entry = jsonc_new_object_entry(key, value);
    if (!new_entry)
        return;
    entry->next = new_entry;
}

void jsonc_obj_insert(JsonObject_t* obj, const char* key, JsonValueType_t ty, void* data)
{
    if (!data && ty != BOOLEAN && ty != NULL_LITERAL)
        return;
    if (!obj || !key)
        return;
    JsonValue_t* value = jsonc_new_value(ty, data);
    jsonc_obj_set(obj, key, value);
}

JsonValue_t* jsonc_obj_get(const JsonObject_t* obj, const char* key)
{
    if (!obj || !key || !obj->entry)
        return NULL;

    JsonObjectEntry_t* current = obj->entry;
    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;
}

const char* jsonc_obj_get_string(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != STRING)
        return NULL;
    return value->string;
}

bool* jsonc_obj_get_bool(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != BOOLEAN)
        return NULL;
    return &value->boolean;
}

double* jsonc_obj_get_number(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != NUMBER)
        return NULL;
    return &value->number;
}

JsonObject_t* jsonc_obj_get_object(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != OBJECT)
        return NULL;
    return value->object;
}

JsonArray_t* jsonc_obj_get_array(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != ARRAY)
        return NULL;
    return value->array;
}

/*
 *   Serialization
 */

typedef struct {
    char* buffer;
    size_t capacity;
    size_t pos;
} StringBuilder_t;

void builder_reset(StringBuilder_t* builder)
{
    memset(builder->buffer, 0, builder->capacity);
    builder->capacity = 0;
    builder->pos = 0;
}

bool builder_resize(StringBuilder_t* builder, size_t capacity)
{
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

bool builder_append_ch(StringBuilder_t* builder, char ch)
{
    if (builder->pos + 1 >= builder->capacity) {
        if (!builder_resize(builder, builder->capacity + 16))
            return false;
    }
    builder->buffer[builder->pos++] = ch;
    return true;
}

bool builder_append(StringBuilder_t* builder, const char* format, ...)
{
    if (!builder || builder->pos >= builder->capacity)
        return false;
    va_list args;
    va_start(args, format);
    int len = vsnprintf(NULL, 0, format, args);
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
            builder_append(builder, "%c", ch);
        }
    }
}
static void builder_serialize_obj(StringBuilder_t*, const JsonObject_t*);
static void builder_serialize_arr(StringBuilder_t*, const JsonArray_t*);

static void builder_serialize_value(StringBuilder_t* builder, const JsonValue_t* value)
{
    if (!builder || !value)
        return;
    switch (value->ty) {
    case STRING:
        builder_append(builder, "\"");
        builder_append_escaped_str(builder, value->string);
        builder_append(builder, "\"");
        break;
    case NUMBER:
        // FIXME: This is hackish
        if ((long long)value->number == value->number) {
            builder_append(builder, "%ld", (long long)value->number);
        } else {
            builder_append(builder, "%f", value->number);
        }
        break;
    case OBJECT:
        builder_serialize_obj(builder, value->object);
        break;
    case ARRAY:
        builder_serialize_arr(builder, value->array);
        break;
    case BOOLEAN:
        builder_append(builder, "%s", value->boolean ? "true" : "false");
        break;
    case NULL_LITERAL:
        builder_append(builder, "null");
        break;
    }
}

void builder_serialize_obj(StringBuilder_t* builder, const JsonObject_t* obj)
{
    JsonObjectEntry_t* current = obj->entry;
    builder_append(builder, "{");
    while (current) {
        builder_append(builder, "\"");
        builder_append_escaped_str(builder, current->key);
        builder_append(builder, "\":");
        if (current->value) {
            builder_serialize_value(builder, current->value);
        } else {
            builder_append(builder, "null");
        }
        if (current->next)
            builder_append(builder, ",");
        current = current->next;
    }
    builder_append(builder, "}");
}

void builder_serialize_arr(StringBuilder_t* builder, const JsonArray_t* obj)
{
    JsonArrayEntry_t* current = obj->entry;
    builder_append(builder, "[");
    while (current) {
        if (current->value) {
            builder_serialize_value(builder, current->value);
        }
        if (current->next)
            builder_append(builder, ",");
        current = current->next;
    }
    builder_append(builder, "]");
}

char* jsonc_doc_to_string(const JsonDocument_t* doc)
{
    if (doc->array && doc->object)
        return NULL;
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 64);
    if (doc->object)
        builder_serialize_obj(&builder, doc->object);
    if (doc->array)
        builder_serialize_arr(&builder, doc->array);
    return builder.buffer;
}

/*
 * Parsing
 */

typedef struct {
    const char* text;
    size_t pos;
    size_t len;
} JsonParser_t;

typedef struct {
    const char* text;
    size_t len;
} StringView_t;

bool parser_eof(const JsonParser_t* parser) { return parser->pos >= parser->len; }

void parser_ignore(JsonParser_t* parser, size_t count)
{
    size_t skipped = count;
    size_t remanining = parser->len - parser->pos;
    if (skipped >= remanining)
        skipped = remanining;
    parser->pos += skipped;
}

char parser_consume(JsonParser_t* parser)
{
    if (parser->pos >= parser->len)
        return EOF;
    return parser->text[parser->pos++];
}

char parser_peek(JsonParser_t* parser, size_t off)
{
    if (parser->pos + off >= parser->len)
        return EOF;
    return parser->text[parser->pos + off];
}

bool parser_next_is(JsonParser_t* parser, char ch)
{
    return parser_peek(parser, 1) == ch;
}

bool parser_consume_specific(JsonParser_t* parser, const char* str)
{
    size_t len = strlen(str);
    for (size_t i = 0; i < len; i++) {
        if (parser_peek(parser, i) != str[i]) {
            return false;
        }
    }
    parser_ignore(parser, len);
    return true;
}

typedef bool (*CharPredicate)(char);
void ignore_while(JsonParser_t* parser, CharPredicate pred)
{
    while (!parser_eof(parser) && pred(parser_peek(parser, 0))) {
        parser_ignore(parser, 1);
    }
}
bool is_space(char ch)
{
    return ch == '\t' || ch == '\n' || ch == '\r' || ch == ' ';
}

static JsonObject_t* parse_obj(JsonParser_t*);
static JsonArray_t* parse_arr(JsonParser_t*);

bool parse_and_unescape_str(JsonParser_t* parser, StringBuilder_t* builder)
{
    if (!parser_consume_specific(parser, "\""))
        return false;

    for (;;) {
        size_t peek_index = parser->pos;
        char ch = 0;
        for (;;) {
            if (peek_index == parser->len)
                break;
            ch = parser->text[peek_index];
            if (ch == '"' || ch == '\\')
                break;
            peek_index++;
        }

        while (peek_index != parser->pos) {
            builder_append_ch(builder, parser_consume(parser));
        }
        if (parser_eof(parser))
            break;
        if (ch == '"')
            break;
        if (ch != '\\') {
            builder_append_ch(builder, parser_consume(parser));
            continue;
        }
        parser_ignore(parser, 1);

        if (parser_next_is(parser, '"')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '"');
            continue;
        }

        if (parser_next_is(parser, '\\')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\\');
            continue;
        }

        if (parser_next_is(parser, '/')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '/');
            continue;
        }

        if (parser_next_is(parser, 'n')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\n');
            continue;
        }

        if (parser_next_is(parser, 'r')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\r');
            continue;
        }

        if (parser_next_is(parser, 't')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\t');
            continue;
        }

        if (parser_next_is(parser, 'b')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\b');
            continue;
        }

        if (parser_next_is(parser, 'f')) {
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\f');
            continue;
        }
    }

    if (!parser_consume_specific(parser, "\""))
        return false;

    return true;
}

JsonValue_t* parse_string(JsonParser_t* parser)
{
    StringBuilder_t builder = { 0 };
    if (!builder_resize(&builder, 64))
        return NULL;
    if (!parse_and_unescape_str(parser, &builder)) {
        free(builder.buffer);
        return NULL;
    }
    JsonValue_t* value = jsonc_new_value(STRING, builder.buffer);
    free(builder.buffer);
    return value;
}

JsonValue_t* parse_true(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "true"))
        return NULL;
    return jsonc_new_value(BOOLEAN, true);
}

JsonValue_t* parse_false(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "false"))
        return NULL;
    return jsonc_new_value(BOOLEAN, false);
}

JsonValue_t* parse_null(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "null"))
        return NULL;
    return jsonc_new_value(NULL_LITERAL, NULL);
}

JsonValue_t* parse_number(JsonParser_t* parser)
{
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 12);

    if (parser_peek(parser, 0) == '-')
        builder_append_ch(&builder, parser_consume(parser));

    if (!isdigit(parser_peek(parser, 0)))
        goto EXIT_ERROR;

    while (isdigit(parser_peek(parser, 0))) {
        builder_append_ch(&builder, parser_consume(parser));
    }

    char ch = parser_peek(parser, 0);
    if (ch != '.')
        goto EXIT_OK;

    builder_append_ch(&builder, parser_consume(parser));
    while (isdigit(parser_peek(parser, 0))) {
        builder_append_ch(&builder, parser_consume(parser));
    }

    ignore_while(parser, is_space);

    ch = parser_peek(parser, 0);
    if (ch != ',' && ch != '}' && ch != ']')
        goto EXIT_ERROR;

EXIT_OK:
    double value = atof(builder.buffer);
    free(builder.buffer);
    return jsonc_new_value(NUMBER, &value);
EXIT_ERROR:
    free(builder.buffer);
    return NULL;
}

JsonValue_t* parse_value(JsonParser_t* parser)
{
    ignore_while(parser, is_space);
    char type_hint = parser_peek(parser, 0);
    switch (type_hint) {
    case '{':
        JsonObject_t* obj = parse_obj(parser);
        if (obj)
            return jsonc_new_value(OBJECT, obj);
        break;
    case '[':
        JsonArray_t* arr = parse_arr(parser);
        if (arr)
            return jsonc_new_value(ARRAY, arr);
        break;
    case '"':
        return parse_string(parser);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return parse_number(parser);
    case 'f':
        return parse_false(parser);
    case 't':
        return parse_true(parser);
    case 'n':
        return parse_null(parser);
    }
    return NULL;
}

JsonObject_t* parse_obj(JsonParser_t* parser)
{
    JsonObject_t* obj = jsonc_new_obj();
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 64);

    if (!parser_consume_specific(parser, "{"))
        goto EXIT_ERROR;

    for (;;) {
        ignore_while(parser, is_space);
        if (parser_peek(parser, 0) == '}')
            break;
        ignore_while(parser, is_space);

        if (!parse_and_unescape_str(parser, &builder))
            goto EXIT_ERROR;
        ignore_while(parser, is_space);

        if (!parser_consume_specific(parser, ":"))
            goto EXIT_ERROR;
        ignore_while(parser, is_space);

        JsonValue_t* value = parse_value(parser);
        if (!value)
            goto EXIT_ERROR;

        jsonc_obj_set(obj, builder.buffer, value);
        builder_reset(&builder);
        ignore_while(parser, is_space);
        if (parser_peek(parser, 0) == '}')
            break;

        if (!parser_consume_specific(parser, ","))
            goto EXIT_ERROR;
        ignore_while(parser, is_space);

        if (parser_peek(parser, 0) == '}')
            goto EXIT_ERROR;
    }

    ignore_while(parser, is_space);
    if (!parser_consume_specific(parser, "}"))
        goto EXIT_ERROR;

    free(builder.buffer);
    return obj;

EXIT_ERROR:
    free(builder.buffer);
    jsonc_free_obj(obj);
    return NULL;
}

JsonArray_t* parse_arr(JsonParser_t* parser)
{
    JsonArray_t* arr = jsonc_new_array();
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 64);

    if (!parser_consume_specific(parser, "["))
        goto EXIT_ERROR;

    for (;;) {
        ignore_while(parser, is_space);
        if (parser_peek(parser, 0) == ']')
            break;

        JsonValue_t* value = parse_value(parser);
        if (!value)
            goto EXIT_ERROR;
        jsonc_array_insert(arr, value);
        builder_reset(&builder);
        ignore_while(parser, is_space);

        if (parser_peek(parser, 0) == ']')
            break;

        if (!parser_consume_specific(parser, ","))
            goto EXIT_ERROR;

        ignore_while(parser, is_space);

        if (parser_peek(parser, 0) == ']')
            goto EXIT_ERROR;
    }

    ignore_while(parser, is_space);
    if (!parser_consume_specific(parser, "]"))
        goto EXIT_ERROR;

    free(builder.buffer);
    return arr;

EXIT_ERROR:
    free(builder.buffer);
    jsonc_free_array(arr);
    return NULL;
}

JsonDocument_t* parse_doc(JsonParser_t* parser)
{
    JsonDocument_t* doc = jsonc_new_doc();

    ignore_while(parser, is_space);
    char type_hint = parser_peek(parser, 0);
    switch (type_hint) {
    case '{': {
        JsonObject_t* obj = parse_obj(parser);
        if (obj) {
            jsonc_doc_set_obj(doc, obj);
            return doc;
        }
        break;
    case '[':
        JsonArray_t* arr = parse_arr(parser);
        if (arr) {
            jsonc_doc_set_array(doc, arr);
            return doc;
        }
        break;
    }
    }

    jsonc_free_doc(doc);
    return NULL;
}

JsonDocument_t* jsonc_doc_from_string(const char* str)
{
    if (!str)
        return NULL;
    JsonParser_t parser = { .text = str, .pos = 0, .len = strlen(str) };
    JsonDocument_t* doc = parse_doc(&parser);
    if (!doc)
        return NULL;
    // Check if all input was consumed
    ignore_while(&parser, is_space);
    if (!parser_eof(&parser)) {
        jsonc_free_doc(doc);
        return NULL;
    }
    return doc;
}