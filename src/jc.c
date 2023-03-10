#include <assert.h>
#include <ctype.h>
#include <jc.h>
#include <olh_map.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string_builder.h>

#ifndef JC_INIT_ARR_CAPACITY
#    define JC_INIT_ARR_CAPACITY 32
#endif

#ifndef JC_INIT_OBJ_CAPACITY
#    define JC_INIT_OBJ_CAPACITY 16
#endif

struct JsonArray_t {
    size_t size;
    size_t capacity;
    JsonValue_t** data;
};

struct JsonObject_t {
    OrderedLinkedHashMap_t olh_map;
};

struct JsonDocument_t {
    JsonObject_t* object;
    JsonArray_t* array;
};

typedef struct {
    const char* text;
    size_t pos;
    size_t len;
} JsonParser_t;

static void builder_serialize_obj(StringBuilder_t*, const JsonObject_t*, size_t, size_t);
static void builder_serialize_arr(StringBuilder_t*, const JsonArray_t*, size_t, size_t);
static JsonObject_t* parse_obj(JsonParser_t*);
static JsonArray_t* parse_arr(JsonParser_t*);

JsonDocument_t* jc_new_doc()
{
    return (JsonDocument_t*)calloc(1, sizeof(JsonDocument_t));
}

JsonObject_t* jc_new_obj()
{
    JsonObject_t* obj = (JsonObject_t*)calloc(1, sizeof(JsonObject_t));
    if (!obj)
        return NULL;
    obj->olh_map.value_free_func = (olh_map_value_free)jc_free_value;
    if (!olh_map_rehash(&obj->olh_map, JC_INIT_OBJ_CAPACITY)) {
        free(obj);
        return NULL;
    }
    return obj;
}

JsonArray_t* jc_new_arr()
{
    JsonArray_t* arr = (JsonArray_t*)calloc(1, sizeof(JsonArray_t));
    if (!arr)
        return NULL;
    arr->data = (JsonValue_t**)calloc(1, JC_INIT_ARR_CAPACITY * sizeof(JsonValue_t*));
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    arr->capacity = JC_INIT_ARR_CAPACITY;
    return arr;
}

JsonValue_t* jc_new_value(JsonValueType_t ty, void* data)
{
    if (!data && ty != JC_NULL_LITERAL && ty != JC_BOOLEAN)
        return NULL;

    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = ty;
    switch (ty) {
    case JC_STRING:
        value->string = strdup(data);
        if (!value->string)
            return NULL;
        break;
    case JC_DOUBLE:
        value->num_double = *(const double*)data;
        break;
    case JC_INT64:
        value->num_int64 = *(const int64_t*)data;
        break;
    case JC_OBJECT:
        value->object = data;
        break;
    case JC_ARRAY:
        value->array = data;
        break;
    case JC_BOOLEAN:
        value->boolean = data != 0 ? true : false;
        break;
    case JC_NULL_LITERAL:
        break;
    }
    return value;
}

JsonValue_t* jc_new_bool_value(bool b)
{
    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = JC_BOOLEAN;
    value->boolean = b;
    return value;
}

JsonValue_t* jc_new_double_value(double dbl)
{
    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = JC_DOUBLE;
    value->num_double = dbl;
    return value;
}

JsonValue_t* jc_new_int64_value(int64_t i64)
{
    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = JC_INT64;
    value->num_int64 = i64;
    return value;
}

void jc_free_doc(JsonDocument_t* doc)
{
    if (!doc)
        return;
    if (doc->object)
        jc_free_obj(doc->object);
    if (doc->array)
        jc_free_arr(doc->array);
    free(doc);
}

void jc_free_obj(JsonObject_t* obj)
{
    if (!obj)
        return;
    olh_map_free(&obj->olh_map);
    free(obj);
}

void jc_free_arr(JsonArray_t* arr)
{
    if (!arr)
        return;
    for (size_t i = 0; i < arr->size; i++)
        jc_free_value(arr->data[i]);
    free(arr->data);
    free(arr);
}

void jc_free_value(JsonValue_t* value)
{
    if (!value)
        return;
    if (value->ty == JC_STRING && value->string) {
        free(value->string);
        value->string = NULL;
    }
    if (value->ty == JC_OBJECT && value->object) {
        jc_free_obj(value->object);
        value->object = NULL;
    }
    if (value->ty == JC_ARRAY && value->array) {
        jc_free_arr(value->array);
        value->array = NULL;
    }
    free(value);
}

bool jc_doc_set_obj(JsonDocument_t* doc, JsonObject_t* obj)
{
    if (!doc || !obj)
        return false;
    if (doc->object && doc->object != obj)
        jc_free_obj(doc->object);
    if (doc->array)
        jc_free_arr(doc->array);
    doc->object = obj;
    return true;
}

bool jc_doc_is_obj(const JsonDocument_t* doc)
{
    return doc->object != NULL;
}

JsonObject_t* jc_doc_get_obj(JsonDocument_t* doc)
{
    return doc->object;
}

bool jc_doc_set_arr(JsonDocument_t* doc, JsonArray_t* arr)
{
    if (!doc || !arr)
        return false;
    if (doc->object)
        jc_free_obj(doc->object);
    if (doc->array && doc->array != arr)
        jc_free_arr(doc->array);
    doc->array = arr;
    return true;
}

bool jc_doc_is_arr(const JsonDocument_t* doc)
{
    return doc->array != NULL;
}

JsonArray_t* jc_doc_get_arr(JsonDocument_t* doc)
{
    return doc->array;
}

bool jc_arr_insert(JsonArray_t* arr, JsonValueType_t ty, void* data)
{
    if (!arr || !data)
        return false;
    JsonValue_t* value = jc_new_value(ty, data);
    if (!value)
        return false;
    if (!jc_arr_insert_value(arr, value)) {
        jc_free_value(value);
        return false;
    }
    return true;
}

bool jc_arr_insert_value(JsonArray_t* arr, JsonValue_t* value)
{
    if (!arr || !value)
        return false;
    if (arr->size + 1 >= arr->capacity) {
        JsonValue_t** new_buffer = (JsonValue_t**)realloc(arr->data, arr->capacity * 2 * sizeof(JsonValue_t*));
        if (!new_buffer)
            return false;
        arr->data = new_buffer;
        arr->capacity = arr->capacity * 2;
    }
    arr->data[arr->size] = value;
    arr->size++;
    return true;
}

size_t jc_arr_size(JsonArray_t* arr)
{
    assert(arr);
    return arr->size;
}

JsonValue_t* jc_arr_at(JsonArray_t* arr, size_t index)
{
    assert(arr);
    if (index >= arr->size)
        return NULL;
    return arr->data[index];
}

bool jc_arr_remove(JsonArray_t* arr, size_t index, size_t count)
{
    assert(arr);
    size_t end = index + count;
    if (index >= arr->size || end >= arr->size)
        return false;

    for (size_t i = index; i < end; i++) {
        if (arr->data[i])
            jc_free_value(arr->data[i]);
    }
    memmove(&arr->data[index], &arr->data[end], (arr->size - count) * sizeof(JsonValue_t*));
    arr->size -= count;
    return true;
}

bool jc_obj_set(JsonObject_t* obj, const char* key, JsonValue_t* value)
{
    if (!obj || !key || !value)
        return false;
    return olh_map_set(&obj->olh_map, key, value);
}

bool jc_obj_insert(JsonObject_t* obj, const char* key, JsonValueType_t ty, void* data)
{
    if (!data && ty != JC_BOOLEAN && ty != JC_NULL_LITERAL)
        return false;
    if (!obj || !key)
        return false;
    JsonValue_t* value = jc_new_value(ty, data);
    if (!value)
        return false;
    jc_obj_set(obj, key, value);
    return true;
}

size_t jc_obj_size(const JsonObject_t* obj)
{
    assert(obj);
    return obj->olh_map.size;
}

bool jc_obj_remove(JsonObject_t* obj, const char* key)
{
    if (!obj || !key)
        return false;
    return olh_map_remove(&obj->olh_map, key);
}

JsonValue_t* jc_obj_get(const JsonObject_t* obj, const char* key)
{
    if (!obj || !key)
        return NULL;
    return (JsonValue_t*)olh_map_get(&obj->olh_map, key);
}

const char* jc_obj_get_string(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jc_obj_get(obj, key);
    if (!value || value->ty != JC_STRING)
        return NULL;
    return value->string;
}

bool* jc_obj_get_bool(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jc_obj_get(obj, key);
    if (!value || value->ty != JC_BOOLEAN)
        return NULL;
    return &value->boolean;
}

bool jc_obj_get_double(const JsonObject_t* obj, const char* key, double* dbl)
{
    JsonValue_t* value = jc_obj_get(obj, key);
    if (!value)
        return false;

    switch (value->ty) {
    case JC_DOUBLE:
        *dbl = value->num_double;
        break;
    case JC_INT64:
        *dbl = (double)value->num_int64;
        break;
    default:
        return false;
    }
    return true;
}

bool jc_obj_get_int64(const JsonObject_t* obj, const char* key, int64_t* i64)
{
    JsonValue_t* value = jc_obj_get(obj, key);
    if (!value)
        return false;

    switch (value->ty) {
    case JC_DOUBLE:
        *i64 = (int64_t)value->num_double;
        break;
    case JC_INT64:
        *i64 = value->num_int64;
        break;
    default:
        return false;
    }
    return true;
}

JsonObject_t* jc_obj_get_obj(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jc_obj_get(obj, key);
    if (!value || value->ty != JC_OBJECT)
        return NULL;
    return value->object;
}

JsonArray_t* jc_obj_get_arr(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jc_obj_get(obj, key);
    if (!value || value->ty != JC_ARRAY)
        return NULL;
    return value->array;
}

JsonObjectIter_t jc_obj_iter(const JsonObject_t* obj)
{
    assert(obj);
    JsonObjectIter_t iter = { .opaque = obj->olh_map.head };
    return iter;
}

bool jc_obj_iter_next(JsonObjectIter_t* iter)
{
    if (!iter || !iter->opaque)
        return false;
    iter->opaque = ((BucketEntry_t*)iter->opaque)->next;
    return true;
}

const char* jc_obj_iter_key(const JsonObjectIter_t* iter)
{
    if (!iter || !iter->opaque)
        return NULL;
    return ((BucketEntry_t*)iter->opaque)->key;
}

JsonValue_t* jc_obj_iter_value(const JsonObjectIter_t* iter)
{
    if (!iter || !iter->opaque)
        return NULL;
    return ((BucketEntry_t*)iter->opaque)->value;
}

/*
 *   Serialization
 */

static void print_indent(StringBuilder_t* builder, size_t spaces_per_indent, size_t indent_level)
{
    builder_append_chrs(builder, ' ', spaces_per_indent * indent_level);
}

static void builder_serialize_value(StringBuilder_t* builder, const JsonValue_t* value, size_t spaces_per_indent, size_t indent_level)
{
    if (!builder || !value)
        return;
    switch (value->ty) {
    case JC_STRING:
        builder_append_ch(builder, '"');
        builder_append_escaped_str(builder, value->string);
        builder_append_ch(builder, '"');
        break;
    case JC_DOUBLE:
        builder_append(builder, "%g", value->num_double);
        break;
    case JC_INT64:
        builder_append(builder, "%ld", value->num_int64);
        break;
    case JC_OBJECT:
        builder_serialize_obj(builder, value->object, spaces_per_indent, indent_level);
        break;
    case JC_ARRAY:
        builder_serialize_arr(builder, value->array, spaces_per_indent, indent_level);
        break;
    case JC_BOOLEAN:
        builder_append(builder, "%s", value->boolean ? "true" : "false");
        break;
    case JC_NULL_LITERAL:
        builder_append(builder, "null");
        break;
    }
}

void builder_serialize_obj(StringBuilder_t* builder, const JsonObject_t* obj, size_t spaces_per_indent, size_t indent_level)
{
    BucketEntry_t* current = obj->olh_map.head;
    builder_append_ch(builder, '{');
    if (spaces_per_indent != 0)
        builder_append_ch(builder, '\n');
    while (current) {
        print_indent(builder, spaces_per_indent, indent_level + 1);
        builder_append_ch(builder, '"');
        builder_append_escaped_str(builder, current->key);
        builder_append_ch(builder, '"');
        builder_append_ch(builder, ':');
        if (spaces_per_indent != 0)
            builder_append_ch(builder, ' ');
        if (current->value) {
            builder_serialize_value(builder, current->value, spaces_per_indent, indent_level + 1);
        } else {
            print_indent(builder, spaces_per_indent, indent_level + 1);
            builder_append(builder, "null");
        }
        if (current->next)
            builder_append_ch(builder, ',');
        if (spaces_per_indent != 0)
            builder_append_ch(builder, '\n');
        current = current->next;
    }
    print_indent(builder, spaces_per_indent, indent_level);
    builder_append_ch(builder, '}');
}

void builder_serialize_arr(StringBuilder_t* builder, const JsonArray_t* arr, size_t spaces_per_indent, size_t indent_level)
{
    builder_append_ch(builder, '[');
    if (spaces_per_indent != 0)
        builder_append_ch(builder, '\n');

    for (size_t i = 0; i < arr->size; i++) {
        JsonValue_t* current = arr->data[i];
        if (current) {
            print_indent(builder, spaces_per_indent, indent_level + 1);
            builder_serialize_value(builder, current, spaces_per_indent, indent_level + 1);
            if (i + 1 < arr->size)
                builder_append_ch(builder, ',');
            if (spaces_per_indent != 0)
                builder_append_ch(builder, '\n');
        }
    }

    print_indent(builder, spaces_per_indent, indent_level);
    builder_append_ch(builder, ']');
}

char* jc_doc_to_string(const JsonDocument_t* doc, size_t spaces_per_indent)
{
    if (doc->array && doc->object)
        return NULL;
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 64);
    if (doc->object)
        builder_serialize_obj(&builder, doc->object, spaces_per_indent, 0);
    if (doc->array)
        builder_serialize_arr(&builder, doc->array, spaces_per_indent, 0);
    return builder.buffer;
}

/*
 * Parsing
 */

static inline bool parser_eof(const JsonParser_t* parser) { return parser->pos >= parser->len; }

static inline size_t parser_remaining(const JsonParser_t* parser) { return parser->len - parser->pos; }

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

static inline bool parser_consume_specific(JsonParser_t* parser, const char* str, size_t len)
{
    if (strncmp(&parser->text[parser->pos], str, len) != 0)
        return false;
    parser_ignore(parser, len);
    return true;
}

static inline bool is_space(char ch)
{
    return ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r';
}

static inline void ignore_whitespace(JsonParser_t* parser)
{
    while (!parser_eof(parser) && is_space(parser_peek(parser, 0))) {
        parser_ignore(parser, 1);
    }
}

static inline bool parse_hex(const char* str, size_t len, uint32_t* value)
{
    for (size_t i = 0; i < len; i++) {
        char digit = str[i];
        uint8_t digit_val;
        if (*value > (UINT32_MAX >> 4))
            return false;

        if (digit >= '0' && digit <= '9') {
            digit_val = (uint8_t)(digit - '0');
        } else if (digit >= 'a' && digit <= 'f') {
            digit_val = 10 + (uint8_t)(digit - 'a');
        } else if (digit >= 'A' && digit <= 'F') {
            digit_val = 10 + (uint8_t)(digit - 'A');
        } else {
            return false;
        }

        *value = (*value << 4) + digit_val;
    }
    return true;
}

static inline bool parse_unicode_symbol(JsonParser_t* parser, StringBuilder_t* builder)
{
    if (!parser_consume_specific(parser, "u", 1))
        return false;
    if (parser_remaining(parser) < 4)
        return false;

    uint32_t code_point = 0;
    if (parse_hex(&parser->text[parser->pos], 4, &code_point)) {
        builder_append_unicode(builder, code_point);
        parser_ignore(parser, 4);
        return true;
    }
    return false;
}

bool parse_and_unescape_str(JsonParser_t* parser, StringBuilder_t* builder)
{
    if (!parser_consume_specific(parser, "\"", 1))
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
            char ch = parser_consume(parser);
            if (ch == '\t' || ch == '\n')
                return false;
            builder_append_ch(builder, ch);
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

        switch (parser_peek(parser, 0)) {
        case '"':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '"');
            continue;
        case '\\':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\\');
            continue;
        case '/':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '/');
            continue;
        case 'n':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\n');
            continue;
        case 'r':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\r');
            continue;
        case 't':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\t');
            continue;
        case 'b':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\b');
            continue;
        case 'f':
            parser_ignore(parser, 1);
            builder_append_ch(builder, '\f');
            continue;
        case 'u': {
            if (parse_unicode_symbol(parser, builder))
                continue;
        }
        }
        return false;
    }

    if (!parser_consume_specific(parser, "\"", 1))
        return false;

    return true;
}

static inline JsonValue_t* parse_string(JsonParser_t* parser)
{
    StringBuilder_t builder = { 0 };
    if (!builder_resize(&builder, 64))
        return NULL;
    if (!parse_and_unescape_str(parser, &builder)) {
        free(builder.buffer);
        return NULL;
    }
    JsonValue_t* value = jc_new_value(JC_STRING, builder.buffer);
    free(builder.buffer);
    return value;
}

static inline JsonValue_t* parse_true(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "true", 4))
        return NULL;
    return jc_new_bool_value(true);
}

static inline JsonValue_t* parse_false(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "false", 5))
        return NULL;
    return jc_new_bool_value(false);
}

static inline JsonValue_t* parse_null(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "null", 4))
        return NULL;
    return jc_new_value(JC_NULL_LITERAL, NULL);
}

JsonValue_t* parse_number(JsonParser_t* parser)
{
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 12);

    /*
        https://www.rfc-editor.org/rfc/rfc4627
        number = [ minus ] int [ frac ] [ exp ]
        exp = e [ minus / plus ] 1*DIGIT
        frac = decimal-point 1*DIGIT
        int = zero | ( digit1-9 *DIGIT )
        minus = %x2D               ; -
        plus = %x2B                ; +
        zero = %x30                ; 0
        decimal-point = %x2E       ; .
        digit1-9 = %x31-39         ; 1-9
        e = %x65 | %x45            ; e E
    */

    // [ minus ]
    char ch = parser_peek(parser, 0);
    if (ch == '-')
        builder_append_ch(&builder, parser_consume(parser));

    // int
    ch = parser_peek(parser, 0);
    if (ch == '0') {
        builder_append_ch(&builder, parser_consume(parser));
    } else if (ch >= '1' && ch <= '9') {
        builder_append_ch(&builder, parser_consume(parser));
        while (isdigit(parser_peek(parser, 0)))
            builder_append_ch(&builder, parser_consume(parser));
    } else {
        goto EXIT_ERROR;
    }

    bool parse_as_double = false;
    // [frac] [exp]
    ch = parser_peek(parser, 0);
    if (ch == '.') {
        // [ frac ]
        parse_as_double = true;
        builder_append_ch(&builder, parser_consume(parser));
        ch = parser_peek(parser, 0);
        if (!isdigit(ch))
            goto EXIT_ERROR;
        builder_append_ch(&builder, parser_consume(parser));
        while (isdigit(parser_peek(parser, 0)))
            builder_append_ch(&builder, parser_consume(parser));
        ch = parser_peek(parser, 0);
    }

    if (ch == 'e' || ch == 'E') {
        // [ exp ]
        parse_as_double = true;
        builder_append_ch(&builder, parser_consume(parser));
        ch = parser_peek(parser, 0);
        if (ch == '+' || ch == '-') {
            builder_append_ch(&builder, parser_consume(parser));
            ch = parser_peek(parser, 0);
            if (ch < '0' || ch > '9')
                goto EXIT_ERROR;
        } else if (ch >= '0' && ch <= '9') {
            builder_append_ch(&builder, parser_consume(parser));
        } else {
            goto EXIT_ERROR;
        }
        while (isdigit(parser_peek(parser, 0)))
            builder_append_ch(&builder, parser_consume(parser));
    }

    JsonValue_t* result = NULL;
    if (parse_as_double) {
        char* end_ptr = NULL;
        double value = strtod(builder.buffer, &end_ptr);
        if (end_ptr != builder.buffer + builder.pos)
            goto EXIT_ERROR;
        result = jc_new_double_value(value);
    } else {
        char* end_ptr = NULL;
        int64_t value = strtoll(builder.buffer, &end_ptr, 10);
        if (end_ptr != builder.buffer + builder.pos)
            goto EXIT_ERROR;
        result = jc_new_int64_value(value);
    }
    free(builder.buffer);
    return result;
EXIT_ERROR:
    free(builder.buffer);
    return NULL;
}

JsonValue_t* parse_value(JsonParser_t* parser)
{
    ignore_whitespace(parser);
    char type_hint = parser_peek(parser, 0);
    switch (type_hint) {
    case '{':
        JsonObject_t* obj = parse_obj(parser);
        if (obj)
            return jc_new_value(JC_OBJECT, obj);
        break;
    case '[':
        JsonArray_t* arr = parse_arr(parser);
        if (arr)
            return jc_new_value(JC_ARRAY, arr);
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
    JsonObject_t* obj = jc_new_obj();
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 64);

    if (!parser_consume_specific(parser, "{", 1))
        goto EXIT_ERROR;

    for (;;) {
        ignore_whitespace(parser);
        if (parser_peek(parser, 0) == '}')
            break;
        ignore_whitespace(parser);

        if (!parse_and_unescape_str(parser, &builder))
            goto EXIT_ERROR;
        ignore_whitespace(parser);

        if (!parser_consume_specific(parser, ":", 1))
            goto EXIT_ERROR;
        ignore_whitespace(parser);

        JsonValue_t* value = parse_value(parser);
        if (!value)
            goto EXIT_ERROR;

        jc_obj_set(obj, builder.buffer, value);
        builder_reset(&builder);
        ignore_whitespace(parser);
        if (parser_peek(parser, 0) == '}')
            break;

        if (!parser_consume_specific(parser, ",", 1))
            goto EXIT_ERROR;
        ignore_whitespace(parser);

        if (parser_peek(parser, 0) == '}')
            goto EXIT_ERROR;
    }

    ignore_whitespace(parser);
    if (!parser_consume_specific(parser, "}", 1))
        goto EXIT_ERROR;

    free(builder.buffer);
    return obj;

EXIT_ERROR:
    free(builder.buffer);
    jc_free_obj(obj);
    return NULL;
}

JsonArray_t* parse_arr(JsonParser_t* parser)
{
    JsonArray_t* arr = jc_new_arr();
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, 64);

    if (!parser_consume_specific(parser, "[", 1))
        goto EXIT_ERROR;

    for (;;) {
        ignore_whitespace(parser);
        if (parser_peek(parser, 0) == ']')
            break;

        JsonValue_t* value = parse_value(parser);
        if (!value)
            goto EXIT_ERROR;
        jc_arr_insert_value(arr, value);
        builder_reset(&builder);
        ignore_whitespace(parser);

        if (parser_peek(parser, 0) == ']')
            break;

        if (!parser_consume_specific(parser, ",", 1))
            goto EXIT_ERROR;

        ignore_whitespace(parser);

        if (parser_peek(parser, 0) == ']')
            goto EXIT_ERROR;
    }

    ignore_whitespace(parser);
    if (!parser_consume_specific(parser, "]", 1))
        goto EXIT_ERROR;

    free(builder.buffer);
    return arr;

EXIT_ERROR:
    free(builder.buffer);
    jc_free_arr(arr);
    return NULL;
}

JsonDocument_t* parse_doc(JsonParser_t* parser)
{
    JsonDocument_t* doc = jc_new_doc();

    ignore_whitespace(parser);
    char type_hint = parser_peek(parser, 0);
    switch (type_hint) {
    case '{': {
        JsonObject_t* obj = parse_obj(parser);
        if (obj) {
            jc_doc_set_obj(doc, obj);
            return doc;
        }
        break;
    case '[':
        JsonArray_t* arr = parse_arr(parser);
        if (arr) {
            jc_doc_set_arr(doc, arr);
            return doc;
        }
        break;
    }
    }

    jc_free_doc(doc);
    return NULL;
}

JsonDocument_t* jc_doc_from_string(const char* str)
{
    if (!str)
        return NULL;
    JsonParser_t parser = { .text = str, .pos = 0, .len = strlen(str) };
    JsonDocument_t* doc = parse_doc(&parser);
    if (!doc)
        return NULL;
    // Check if all input was consumed
    ignore_whitespace(&parser);
    if (!parser_eof(&parser)) {
        jc_free_doc(doc);
        return NULL;
    }
    return doc;
}
