#ifndef JSONC__
#define JSONC__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct JsonObject_t JsonObject_t;
typedef struct JsonObjectEntry_t JsonObjectEntry_t;

typedef struct JsonArray_t JsonArray_t;

typedef enum {
    JSONC_STRING,
    JSONC_NUMBER,
    JSONC_OBJECT,
    JSONC_ARRAY,
    JSONC_BOOLEAN,
    JSONC_NULL_LITERAL,
} JsonValueType_t;

typedef struct {
    union {
        char* string;
        double number;
        bool boolean;
        JsonObject_t* object;
        JsonArray_t* array;
    };
    JsonValueType_t ty;
} JsonValue_t;

struct JsonArray_t {
    size_t size;
    size_t capacity;
    JsonValue_t** data;
};

struct JsonObjectEntry_t {
    char* key;
    JsonValue_t* value;
    JsonObjectEntry_t* next;
};

struct JsonObject_t {
    JsonObjectEntry_t* entry;
};

typedef struct {
    JsonObject_t* object;
    JsonArray_t* array;
} JsonDocument_t;

JsonDocument_t* jsonc_new_doc();
JsonObject_t* jsonc_new_obj();
JsonArray_t* jsonc_new_arr();
JsonValue_t* jsonc_new_value(JsonValueType_t ty, void* data);
JsonValue_t* jsonc_new_value_bool(bool);

void jsonc_free_doc(JsonDocument_t* doc);
void jsonc_free_obj(JsonObject_t* obj);
void jsonc_free_arr(JsonArray_t* arr);
void jsonc_free_value(JsonValue_t* value);

bool jsonc_doc_set_obj(JsonDocument_t* doc, JsonObject_t* obj);
bool jsonc_doc_is_obj(const JsonDocument_t* doc);
bool jsonc_doc_set_arr(JsonDocument_t* doc, JsonArray_t* obj);
bool jsonc_doc_is_arr(const JsonDocument_t* doc);

bool jsonc_arr_insert_value(JsonArray_t* arr, JsonValue_t* value);
bool jsonc_arr_insert(JsonArray_t* arr, JsonValueType_t ty, void* data);

bool jsonc_obj_set(JsonObject_t* obj, const char* key, JsonValue_t* value);
bool jsonc_obj_insert(JsonObject_t* obj, const char* key, JsonValueType_t ty, void* data);
bool jsonc_obj_remove(JsonObject_t* obj, const char* key);

JsonValue_t* jsonc_obj_get(const JsonObject_t* obj, const char* key);
const char* jsonc_obj_get_string(const JsonObject_t* obj, const char* key);
bool* jsonc_obj_get_bool(const JsonObject_t* obj, const char* key);
double* jsonc_obj_get_number(const JsonObject_t* obj, const char* key);
JsonObject_t* jsonc_obj_get_obj(const JsonObject_t* obj, const char* key);
JsonArray_t* jsonc_obj_get_arr(const JsonObject_t* obj, const char* key);

char* jsonc_doc_to_string(const JsonDocument_t* doc, size_t spaces_per_indent);
JsonDocument_t* jsonc_doc_from_string(const char* str);

#endif