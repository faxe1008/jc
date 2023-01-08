#ifndef LIB_JC__
#define LIB_JC__
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct JsonArray_t JsonArray_t;
typedef struct JsonObject_t JsonObject_t;
typedef struct JsonDocument_t JsonDocument_t;

typedef enum {
    JC_STRING,
    JC_NUMBER,
    JC_OBJECT,
    JC_ARRAY,
    JC_BOOLEAN,
    JC_NULL_LITERAL,
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

typedef struct {
    void* opaque;
} JsonObjectIter_t;

JsonDocument_t* jc_new_doc();
JsonObject_t* jc_new_obj();
JsonArray_t* jc_new_arr();
JsonValue_t* jc_new_value(JsonValueType_t ty, void* data);
JsonValue_t* jc_new_value_bool(bool);

void jc_free_doc(JsonDocument_t* doc);
void jc_free_obj(JsonObject_t* obj);
void jc_free_arr(JsonArray_t* arr);
void jc_free_value(JsonValue_t* value);

bool jc_doc_set_obj(JsonDocument_t* doc, JsonObject_t* obj);
bool jc_doc_is_obj(const JsonDocument_t* doc);
JsonObject_t* jc_doc_get_obj(JsonDocument_t* doc);

bool jc_doc_set_arr(JsonDocument_t* doc, JsonArray_t* obj);
bool jc_doc_is_arr(const JsonDocument_t* doc);
JsonArray_t* jc_doc_get_arr(JsonDocument_t* doc);

bool jc_arr_insert_value(JsonArray_t* arr, JsonValue_t* value);
bool jc_arr_insert(JsonArray_t* arr, JsonValueType_t ty, void* data);
size_t jc_arr_size(JsonArray_t* arr);
JsonValue_t* jc_arr_at(JsonArray_t* arr, size_t index);

bool jc_obj_set(JsonObject_t* obj, const char* key, JsonValue_t* value);
bool jc_obj_insert(JsonObject_t* obj, const char* key, JsonValueType_t ty, void* data);
bool jc_obj_remove(JsonObject_t* obj, const char* key);
size_t jc_obj_size(const JsonObject_t* obj);

JsonValue_t* jc_obj_get(const JsonObject_t* obj, const char* key);
const char* jc_obj_get_string(const JsonObject_t* obj, const char* key);
bool* jc_obj_get_bool(const JsonObject_t* obj, const char* key);
double* jc_obj_get_number(const JsonObject_t* obj, const char* key);
JsonObject_t* jc_obj_get_obj(const JsonObject_t* obj, const char* key);
JsonArray_t* jc_obj_get_arr(const JsonObject_t* obj, const char* key);

char* jc_doc_to_string(const JsonDocument_t* doc, size_t spaces_per_indent);
JsonDocument_t* jc_doc_from_string(const char* str);

JsonObjectIter_t jc_obj_iter(const JsonObject_t* obj);
bool jc_obj_iter_next(JsonObjectIter_t* iter);
const char* jc_obj_iter_key(const JsonObjectIter_t* iter);
JsonValue_t* jc_obj_iter_value(const JsonObjectIter_t* iter);

#define jc_obj_foreach(obj, key, value)                 \
    JsonObjectIter_t iter##obj = jc_obj_iter(obj);      \
    const char* key = jc_obj_iter_key(&iter##obj);      \
    JsonValue_t* value = jc_obj_iter_value(&iter##obj); \
    for (; jc_obj_iter_next(&iter##obj);                \
         key = jc_obj_iter_key(&iter##obj), value = jc_obj_iter_value(&iter##obj))

#define jc_arr_foreach(arr, value)          \
    size_t len##arr = jc_arr_size(arr);     \
    JsonValue_t* value = jc_arr_at(arr, 0); \
    for (size_t loopv##arr = 0; loopv##arr < len##arr; loopv##arr++, value = jc_arr_at(arr, loopv##arr))

#endif