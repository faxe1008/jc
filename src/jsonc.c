#include <assert.h>
#include <ctype.h>
#include <jsonc.h>
#include <olh_map.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string_builder.h>

#ifndef JSONC_INIT_ARR_CAPACITY
#    define JSONC_INIT_ARR_CAPACITY 4
#endif

#ifndef JSONC_INIT_OBJ_CAPACITY
#    define JSONC_INIT_OBJ_CAPACITY 4
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

JsonDocument_t* jsonc_new_doc()
{
    return (JsonDocument_t*)calloc(1, sizeof(JsonDocument_t));
}

JsonObject_t* jsonc_new_obj()
{
    JsonObject_t* obj = (JsonObject_t*)calloc(1, sizeof(JsonObject_t));
    if (!obj)
        return NULL;
    obj->olh_map.value_free_func = (olh_map_value_free)jsonc_free_value;
    if (!olh_map_rehash(&obj->olh_map, JSONC_INIT_OBJ_CAPACITY)) {
        free(obj);
        return NULL;
    }
    return obj;
}

JsonArray_t* jsonc_new_arr()
{
    JsonArray_t* arr = (JsonArray_t*)calloc(1, sizeof(JsonArray_t));
    if (!arr)
        return NULL;
    arr->data = (JsonValue_t**)calloc(1, JSONC_INIT_ARR_CAPACITY * sizeof(JsonValue_t*));
    if (!arr->data) {
        free(arr);
        return NULL;
    }
    arr->capacity = JSONC_INIT_ARR_CAPACITY;
    return arr;
}

JsonValue_t* jsonc_new_value(JsonValueType_t ty, void* data)
{
    if (!data && ty != JSONC_NULL_LITERAL && ty != JSONC_BOOLEAN)
        return NULL;

    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = ty;
    switch (ty) {
    case JSONC_STRING:
        value->string = strdup(data);
        if (!value->string)
            return NULL;
        break;
    case JSONC_NUMBER:
        value->number = *(const double*)data;
        break;
    case JSONC_OBJECT:
        value->object = data;
        break;
    case JSONC_ARRAY:
        value->array = data;
        break;
    case JSONC_BOOLEAN:
        value->boolean = data != 0 ? true : false;
        break;
    case JSONC_NULL_LITERAL:
        break;
    }
    return value;
}

JsonValue_t* jsonc_new_value_bool(bool b)
{
    JsonValue_t* value = (JsonValue_t*)calloc(1, sizeof(JsonValue_t));
    if (!value)
        return NULL;
    value->ty = JSONC_BOOLEAN;
    value->boolean = b;
    return value;
}

void jsonc_free_doc(JsonDocument_t* doc)
{
    if (!doc)
        return;
    if (doc->object)
        jsonc_free_obj(doc->object);
    if (doc->array)
        jsonc_free_arr(doc->array);
    free(doc);
}

void jsonc_free_obj(JsonObject_t* obj)
{
    if (!obj)
        return;
    olh_map_free(&obj->olh_map);
    free(obj);
}

void jsonc_free_arr(JsonArray_t* arr)
{
    if (!arr)
        return;
    for (size_t i = 0; i < arr->size; i++)
        jsonc_free_value(arr->data[i]);
    free(arr->data);
    free(arr);
}

void jsonc_free_value(JsonValue_t* value)
{
    if (!value)
        return;
    if (value->ty == JSONC_STRING && value->string) {
        free(value->string);
        value->string = NULL;
    }
    if (value->ty == JSONC_OBJECT && value->object) {
        jsonc_free_obj(value->object);
        value->object = NULL;
    }
    if (value->ty == JSONC_ARRAY && value->array) {
        jsonc_free_arr(value->array);
        value->array = NULL;
    }
    free(value);
}

bool jsonc_doc_set_obj(JsonDocument_t* doc, JsonObject_t* obj)
{
    if (!doc || !obj)
        return false;
    if (doc->object && doc->object != obj)
        jsonc_free_obj(doc->object);
    if (doc->array)
        jsonc_free_arr(doc->array);
    doc->object = obj;
    return true;
}

bool jsonc_doc_is_obj(const JsonDocument_t* doc)
{
    return doc->object != NULL;
}

JsonObject_t* jsonc_doc_get_obj(JsonDocument_t* doc) {
    return doc->object;
}

bool jsonc_doc_set_arr(JsonDocument_t* doc, JsonArray_t* arr)
{
    if (!doc || !arr)
        return false;
    if (doc->object)
        jsonc_free_obj(doc->object);
    if (doc->array && doc->array != arr)
        jsonc_free_arr(doc->array);
    doc->array = arr;
    return true;
}

bool jsonc_doc_is_arr(const JsonDocument_t* doc)
{
    return doc->array != NULL;
}

JsonArray_t* jsonc_doc_get_arr(JsonDocument_t* doc){
    return doc->array;
}

bool jsonc_arr_insert(JsonArray_t* arr, JsonValueType_t ty, void* data)
{
    if (!arr || !data)
        return false;
    JsonValue_t* value = jsonc_new_value(ty, data);
    if (!value)
        return false;
    if (!jsonc_arr_insert_value(arr, value)) {
        jsonc_free_value(value);
        return false;
    }
    return true;
}

bool jsonc_arr_insert_value(JsonArray_t* arr, JsonValue_t* value)
{
    if (!arr || !value)
        return false;
    if (arr->size + 1 >= arr->capacity) {
        JsonValue_t** new_buffer = (JsonValue_t**)calloc(1, arr->capacity * 2 * sizeof(JsonValue_t*));
        if (!new_buffer)
            return false;
        memcpy(new_buffer, arr->data, arr->size * sizeof(JsonArray_t*));
        free(arr->data);
        arr->data = new_buffer;
        arr->capacity = arr->capacity * 2;
    }
    arr->data[arr->size] = value;
    arr->size++;
    return true;
}

bool jsonc_obj_set(JsonObject_t* obj, const char* key, JsonValue_t* value)
{
    if (!obj || !key || !value)
        return false;
    return olh_map_set(&obj->olh_map, key, value);
}

bool jsonc_obj_insert(JsonObject_t* obj, const char* key, JsonValueType_t ty, void* data)
{
    if (!data && ty != JSONC_BOOLEAN && ty != JSONC_NULL_LITERAL)
        return false;
    if (!obj || !key)
        return false;
    JsonValue_t* value = jsonc_new_value(ty, data);
    if (!value)
        return false;
    jsonc_obj_set(obj, key, value);
    return true;
}

bool jsonc_obj_remove(JsonObject_t* obj, const char* key)
{
    if (!obj || !key)
        return false;
    return olh_map_remove(&obj->olh_map, key);
}

JsonValue_t* jsonc_obj_get(const JsonObject_t* obj, const char* key)
{
    if (!obj || !key)
        return NULL;
    return (JsonValue_t*)olh_map_get(&obj->olh_map, key);
}

const char* jsonc_obj_get_string(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != JSONC_STRING)
        return NULL;
    return value->string;
}

bool* jsonc_obj_get_bool(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != JSONC_BOOLEAN)
        return NULL;
    return &value->boolean;
}

double* jsonc_obj_get_number(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != JSONC_NUMBER)
        return NULL;
    return &value->number;
}

JsonObject_t* jsonc_obj_get_obj(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != JSONC_OBJECT)
        return NULL;
    return value->object;
}

JsonArray_t* jsonc_obj_get_arr(const JsonObject_t* obj, const char* key)
{
    JsonValue_t* value = jsonc_obj_get(obj, key);
    if (!value || value->ty != JSONC_ARRAY)
        return NULL;
    return value->array;
}

JsonObjectIter_t jsonc_obj_iter(const JsonObject_t* obj)
{
    assert(obj);
    JsonObjectIter_t iter = { .opaque = obj->olh_map.head };
    return iter;
}

bool jsonc_obj_iter_next(JsonObjectIter_t* iter)
{
    if (!iter || !iter->opaque)
        return false;
    iter->opaque = ((BucketEntry_t*)iter->opaque)->next;
    return true;
}

const char* jsonc_obj_iter_key(const JsonObjectIter_t* iter)
{
    if (!iter || !iter->opaque)
        return NULL;
    return ((BucketEntry_t*)iter->opaque)->key;
}

JsonValue_t* jsonc_obj_iter_value(const JsonObjectIter_t* iter)
{
    if (!iter || !iter->opaque)
        return NULL;
    return ((BucketEntry_t*)iter->opaque)->value;
}

JsonArrayIter_t jsonc_arr_iter(const JsonArray_t* arr){
    assert(arr);
    JsonArrayIter_t iter = {.it = arr->data};
    return iter;
}

bool jsonc_arr_iter_next(JsonArrayIter_t* iter){
    if(!iter || !iter->it)
        return false;
    iter->it++;
    return iter->it != NULL;
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
    case JSONC_STRING:
        builder_append_ch(builder, '"');
        builder_append_escaped_str(builder, value->string);
        builder_append_ch(builder, '"');
        break;
    case JSONC_NUMBER:
        builder_append(builder, "%g", value->number);
        break;
    case JSONC_OBJECT:
        builder_serialize_obj(builder, value->object, spaces_per_indent, indent_level);
        break;
    case JSONC_ARRAY:
        builder_serialize_arr(builder, value->array, spaces_per_indent, indent_level);
        break;
    case JSONC_BOOLEAN:
        builder_append(builder, "%s", value->boolean ? "true" : "false");
        break;
    case JSONC_NULL_LITERAL:
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

char* jsonc_doc_to_string(const JsonDocument_t* doc, size_t spaces_per_indent)
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

bool parser_next_is(JsonParser_t* parser, char ch)
{
    return parser_peek(parser, 0) == ch;
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

static inline bool parse_hex(const char* str, size_t len)
{
    uint32_t value = 0;
    for (size_t i = 0; i < len; i++) {
        char digit = str[i];
        uint8_t digit_val;
        if (value > (UINT32_MAX >> 4))
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

        value = (value << 4) + digit_val;
    }
    return true;
}

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

        if (parser_next_is(parser, 'u')) {
            parser_ignore(parser, 1);

            if (parser_remaining(parser) < 4)
                return false;

            if (parse_hex(&parser->text[parser->pos], 4)) {
                builder_append_ch(builder, parser_consume(parser));
                builder_append_ch(builder, parser_consume(parser));
                builder_append_ch(builder, parser_consume(parser));
                builder_append_ch(builder, parser_consume(parser));
                continue;
            }
            return false;
        }

        return false;
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
    JsonValue_t* value = jsonc_new_value(JSONC_STRING, builder.buffer);
    free(builder.buffer);
    return value;
}

JsonValue_t* parse_true(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "true"))
        return NULL;
    return jsonc_new_value_bool(true);
}

JsonValue_t* parse_false(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "false"))
        return NULL;
    return jsonc_new_value_bool(false);
}

JsonValue_t* parse_null(JsonParser_t* parser)
{
    if (!parser_consume_specific(parser, "null"))
        return NULL;
    return jsonc_new_value(JSONC_NULL_LITERAL, NULL);
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

    // [frac] [exp]
    ch = parser_peek(parser, 0);
    if (ch == '.') {
        // [ frac ]
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

    double value = atof(builder.buffer);
    free(builder.buffer);
    return jsonc_new_value(JSONC_NUMBER, &value);
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
            return jsonc_new_value(JSONC_OBJECT, obj);
        break;
    case '[':
        JsonArray_t* arr = parse_arr(parser);
        if (arr)
            return jsonc_new_value(JSONC_ARRAY, arr);
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
    JsonArray_t* arr = jsonc_new_arr();
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
        jsonc_arr_insert_value(arr, value);
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
    jsonc_free_arr(arr);
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
            jsonc_doc_set_arr(doc, arr);
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