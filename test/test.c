#include <clonk.h>
#include <jsonc.h>

TEST_CASE(set_and_get, {
    JsonObject_t* obj = jsonc_new_obj();

    JsonValue_t* value = jsonc_new_value(JSONC_STRING, "VALUE");
    jsonc_obj_set(obj, "KEY", value);

    VERIFY(jsonc_obj_get(obj, "KEY") == value);

    const char* str_val = jsonc_obj_get_string(obj, "KEY");
    VERIFY(strcmp(str_val, "VALUE") == 0);

    jsonc_free_obj(obj);
});

TEST_CASE(replace_value, {
    JsonObject_t* obj = jsonc_new_obj();

    JsonValue_t* value = jsonc_new_value(JSONC_STRING, "VALUE");
    jsonc_obj_set(obj, "KEY", value);

    JsonValue_t* new_value = jsonc_new_value(JSONC_BOOLEAN, false);
    jsonc_obj_set(obj, "KEY", new_value);
    VERIFY(jsonc_obj_get(obj, "KEY") == new_value);

    jsonc_free_obj(obj);
})

TEST_CASE(serialize_obj, {
    JsonObject_t* obj = jsonc_new_obj();
    jsonc_obj_insert(obj, "key", JSONC_STRING, "value");

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_obj(doc, obj);

    char* serialized = jsonc_doc_to_string(doc, 0);

    VERIFY(strcmp(serialized, "{\"key\":\"value\"}") == 0);
    free(serialized);
    jsonc_free_doc(doc);
})

TEST_CASE(serialize_arr, {
    JsonArray_t* arr = jsonc_new_arr();

    jsonc_arr_insert(arr, JSONC_STRING, "Item1");
    JsonValue_t* v2 = jsonc_new_value_bool(true);
    jsonc_arr_insert_value(arr, v2);

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_arr(doc, arr);

    char* serialized = jsonc_doc_to_string(doc, 0);

    VERIFY(strcmp(serialized, "[\"Item1\",true]") == 0);
    free(serialized);
    jsonc_free_doc(doc);
})

TEST_CASE(serialize_complex, {
    JsonObject_t* obj = jsonc_new_obj();

    jsonc_obj_insert(obj, "key", JSONC_STRING, "value");
    jsonc_obj_set(obj, "boolean_true", jsonc_new_value_bool(true));
    jsonc_obj_set(obj, "boolean_false", jsonc_new_value_bool(false));
    jsonc_obj_set(obj, "NULL", jsonc_new_value(JSONC_NULL_LITERAL, NULL));

    double num = 2.0;
    JsonArray_t* arr = jsonc_new_arr();
    jsonc_arr_insert_value(arr, jsonc_new_value(JSONC_STRING, "Item1"));
    jsonc_arr_insert_value(arr, jsonc_new_value(JSONC_NUMBER, &num));
    jsonc_obj_set(obj, "array", jsonc_new_value(JSONC_ARRAY, arr));

    JsonObject_t* sub = jsonc_new_obj();
    jsonc_obj_insert(sub, "subkey", JSONC_STRING, "subvalue");
    jsonc_obj_set(obj, "subobject", jsonc_new_value(JSONC_OBJECT, sub));

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_obj(doc, obj);

    char* serialized = jsonc_doc_to_string(doc, 0);
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
        char* serialized = jsonc_doc_to_string(doc, 0);
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

TEST_CASE(remove_obj, {
    JsonDocument_t* doc = jsonc_doc_from_string("{\"key\":\"value\"}");
    VERIFY(doc && jsonc_obj_get_string(doc->object, "key"));
    jsonc_obj_remove(doc->object, "key");
    VERIFY(!jsonc_obj_get_string(doc->object, "key"));
    jsonc_free_doc(doc);
});

int main(int argc, char** argv)
{
    REGISTER_TEST_CASE(set_and_get);
    REGISTER_TEST_CASE(replace_value);
    REGISTER_TEST_CASE(serialize_obj);
    REGISTER_TEST_CASE(serialize_arr);
    REGISTER_TEST_CASE(serialize_complex);
    REGISTER_TEST_CASE(serde_valid);
    REGISTER_TEST_CASE(serde_invalid);
    REGISTER_TEST_CASE(remove_obj);
    RUN_TEST_SUITE(argc, argv);
}