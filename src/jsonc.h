#ifndef JSONC__
#define JSONC__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct JsonObject_t JsonObject_t;
typedef struct JsonObjectEntry_t JsonObjectEntry_t;

typedef struct JsonArray_t JsonArray_t;
typedef struct JsonArrayEntry_t JsonArrayEntry_t;

typedef enum {
    STRING,
    NUMBER,
    OBJECT,
    ARRAY,
    BOOLEAN,
    NULL_LITERAL,
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

struct JsonArrayEntry_t {
    JsonValue_t* value;
    JsonArrayEntry_t* next;
};

struct JsonArray_t {
    JsonArrayEntry_t* entry;
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
JsonArray_t* jsonc_new_array();
JsonValue_t* jsonc_new_value(JsonValueType_t ty, void* data);

void jsonc_free_doc(JsonDocument_t* doc);
void jsonc_free_obj(JsonObject_t* obj);
void jsonc_free_array(JsonArray_t* arr);
void jsonc_free_value(JsonValue_t* value);
void jsonc_free_array_entry(JsonArrayEntry_t* entry);
void jsonc_free_obj_entry(JsonObjectEntry_t* entry);

void jsonc_doc_set_obj(JsonDocument_t* doc, JsonObject_t* obj);
void jsonc_doc_set_array(JsonDocument_t* doc, JsonArray_t* obj);

void jsonc_array_insert(JsonArray_t* arr, JsonValue_t* value);
void jsonc_obj_set(JsonObject_t* obj, const char* key, JsonValue_t* value);
void jsonc_obj_insert(JsonObject_t* obj, const char* key, JsonValueType_t ty, void* data);

JsonValue_t* jsonc_obj_get(const JsonObject_t* obj, const char* key);
const char* jsonc_obj_get_string(const JsonObject_t* obj, const char* key);
bool* jsonc_obj_get_bool(const JsonObject_t* obj, const char* key);
double* jsonc_obj_get_number(const JsonObject_t* obj, const char* key);
JsonObject_t* jsonc_obj_get_object(const JsonObject_t* obj, const char* key);
JsonArray_t* jsonc_obj_get_array(const JsonObject_t* obj, const char* key);

char* jsonc_doc_to_string(const JsonDocument_t* doc);

#endif