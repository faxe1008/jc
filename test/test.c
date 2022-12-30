#include <clonk.h>
#include <jsonc.h>

TEST_CASE(set_and_get, {
    JsonObject_t* obj = jsonc_new_obj();

    JsonValue_t* value = jsonc_new_value(STRING, "General Kenobi");
    jsonc_obj_set(obj, "Hello There", value);

    VERIFY(jsonc_obj_get(obj, "Hello There") == value);

    const char* str_val = jsonc_obj_get_string(obj, "Hello There");
    VERIFY(strcmp(str_val, "General Kenobi") == 0);

    jsonc_free_obj(obj);
});

TEST_CASE(replace_value, {
    JsonObject_t* obj = jsonc_new_obj();

    JsonValue_t* value = jsonc_new_value(STRING, "General Kenobi");
    jsonc_obj_set(obj, "Hello There", value);

    JsonValue_t* new_value = jsonc_new_value(BOOLEAN, false);
    jsonc_obj_set(obj, "Hello There", new_value);
    VERIFY(jsonc_obj_get(obj, "Hello There") == new_value);

    jsonc_free_obj(obj);
})

TEST_CASE(serialize_obj, {
    JsonObject_t* obj = jsonc_new_obj();
    jsonc_obj_insert(obj, "key", STRING, "value");

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_obj(doc, obj);

    char* serialized = jsonc_doc_to_string(doc);

    VERIFY(strcmp(serialized, "{\"key\":\"value\"}") == 0);
    free(serialized);
    jsonc_free_doc(doc);
})

TEST_CASE(serialize_arr, {
    JsonArray_t* arr = jsonc_new_array();

    JsonValue_t* v1 = jsonc_new_value(STRING, "Item1");
    JsonValue_t* v2 = jsonc_new_value_bool(true);

    jsonc_array_insert(arr, v1);
    jsonc_array_insert(arr, v2);

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_array(doc, arr);

    char* serialized = jsonc_doc_to_string(doc);

    VERIFY(strcmp(serialized, "[\"Item1\",true]") == 0);
    free(serialized);
    jsonc_free_doc(doc);
})

TEST_CASE(serialize_complex, {
    JsonObject_t* obj = jsonc_new_obj();

    jsonc_obj_insert(obj, "key", STRING, "value");
    jsonc_obj_set(obj, "boolean_true", jsonc_new_value_bool(true));
    jsonc_obj_set(obj, "boolean_false", jsonc_new_value_bool(false));
    jsonc_obj_set(obj, "NULL", jsonc_new_value(NULL_LITERAL, NULL));

    double num = 2.0;
    JsonArray_t* arr = jsonc_new_array();
    jsonc_array_insert(arr, jsonc_new_value(STRING, "Item1"));
    jsonc_array_insert(arr, jsonc_new_value(NUMBER, &num));
    jsonc_obj_set(obj, "array", jsonc_new_value(ARRAY, arr));

    JsonObject_t* sub = jsonc_new_obj();
    jsonc_obj_insert(sub, "subkey", STRING, "subvalue");
    jsonc_obj_set(obj, "subobject", jsonc_new_value(OBJECT, sub));

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_obj(doc, obj);

    char* serialized = jsonc_doc_to_string(doc);
    VERIFY(strcmp(serialized, "{\"key\":\"value\",\"boolean_true\":true,\"boolean_false\":false,\"NULL\":null,\"array\":[\"Item1\",2],\"subobject\":{\"subkey\":\"subvalue\"}}") == 0);
    free(serialized);
    jsonc_free_doc(doc);
})

static const char* valid_docs[] = {
    "[]",
    "{}",
    "[{}]",
    "[2,2.4,true,false,null,\"hello\"]",
    "{\"key\":\"value\"}",
    "{\"key\":\"value\",\"boolean_true\":true,\"boolean_false\":false,\"NULL\":null,\"array\":[\"Item1\",2],\"subobject\":{\"subkey\":\"subvalue\"}}",
    NULL
};

TEST_CASE(serde_valid, {
    for (size_t i = 0; valid_docs[i]; i++) {
        JsonDocument_t* doc = jsonc_doc_from_string(valid_docs[i]);
        VERIFY(doc);
        char* serialized = jsonc_doc_to_string(doc);
        VERIFY(strcmp(valid_docs[i], serialized) == 0);
        free(serialized);
        jsonc_free_doc(doc);
    }
})

static const char* invalid_docs[] = {
    "[NULL]",
    "[01]",
    "[frue]",
    "{\"missing_quote:\"\"}",
    NULL
};

TEST_CASE(serde_invalid, {
    for (size_t i = 0; invalid_docs[i]; i++) {
        JsonDocument_t* doc = jsonc_doc_from_string(invalid_docs[i]);
        VERIFY(!doc);
    }
})

int main(int argc, char** argv)
{
    REGISTER_TEST_CASE(set_and_get);
    REGISTER_TEST_CASE(replace_value);
    REGISTER_TEST_CASE(serialize_obj);
    REGISTER_TEST_CASE(serialize_arr);
    REGISTER_TEST_CASE(serialize_complex);
    REGISTER_TEST_CASE(serde_valid);
    REGISTER_TEST_CASE(serde_invalid);
    RUN_TEST_SUITE(argc, argv);
}