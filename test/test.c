#include <clonk.h>
#include <jc.h>

TEST_CASE(set_and_get, {
    JsonObject_t* obj = jc_new_obj();

    JsonValue_t* value = jc_new_value(JC_STRING, "VALUE");
    jc_obj_set(obj, "KEY", value);

    VERIFY(jc_obj_get(obj, "KEY") == value);

    const char* str_val = jc_obj_get_string(obj, "KEY");
    VERIFY(strcmp(str_val, "VALUE") == 0);

    jc_free_obj(obj);
});

TEST_CASE(replace_value, {
    JsonObject_t* obj = jc_new_obj();

    JsonValue_t* value = jc_new_value(JC_STRING, "VALUE");
    jc_obj_set(obj, "KEY", value);

    JsonValue_t* new_value = jc_new_value(JC_BOOLEAN, false);
    jc_obj_set(obj, "KEY", new_value);
    VERIFY(jc_obj_get(obj, "KEY") == new_value);

    jc_free_obj(obj);
})

TEST_CASE(serialize_obj, {
    JsonObject_t* obj = jc_new_obj();
    jc_obj_insert(obj, "key", JC_STRING, "value");

    JsonDocument_t* doc = jc_new_doc();
    jc_doc_set_obj(doc, obj);

    char* serialized = jc_doc_to_string(doc, 0);

    VERIFY(strcmp(serialized, "{\"key\":\"value\"}") == 0);
    free(serialized);
    jc_free_doc(doc);
})

TEST_CASE(serialize_arr, {
    JsonArray_t* arr = jc_new_arr();

    jc_arr_insert(arr, JC_STRING, "Item1");
    JsonValue_t* v2 = jc_new_value_bool(true);
    jc_arr_insert_value(arr, v2);

    JsonDocument_t* doc = jc_new_doc();
    jc_doc_set_arr(doc, arr);

    char* serialized = jc_doc_to_string(doc, 0);

    VERIFY(strcmp(serialized, "[\"Item1\",true]") == 0);
    free(serialized);
    jc_free_doc(doc);
})

TEST_CASE(serialize_complex, {
    JsonObject_t* obj = jc_new_obj();

    jc_obj_insert(obj, "key", JC_STRING, "value");
    jc_obj_set(obj, "boolean_true", jc_new_value_bool(true));
    jc_obj_set(obj, "boolean_false", jc_new_value_bool(false));
    jc_obj_set(obj, "NULL", jc_new_value(JC_NULL_LITERAL, NULL));

    double num = 2.0;
    JsonArray_t* arr = jc_new_arr();
    jc_arr_insert_value(arr, jc_new_value(JC_STRING, "Item1"));
    jc_arr_insert_value(arr, jc_new_value(JC_NUMBER, &num));
    jc_obj_set(obj, "array", jc_new_value(JC_ARRAY, arr));

    JsonObject_t* sub = jc_new_obj();
    jc_obj_insert(sub, "subkey", JC_STRING, "subvalue");
    jc_obj_set(obj, "subobject", jc_new_value(JC_OBJECT, sub));

    JsonDocument_t* doc = jc_new_doc();
    jc_doc_set_obj(doc, obj);

    char* serialized = jc_doc_to_string(doc, 0);
    VERIFY(strcmp(serialized, "{\"key\":\"value\",\"boolean_true\":true,\"boolean_false\":false,\"NULL\":null,\"array\":[\"Item1\",2],\"subobject\":{\"subkey\":\"subvalue\"}}") == 0);
    free(serialized);
    jc_free_doc(doc);
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
        JsonDocument_t* doc = jc_doc_from_string(valid_docs[i]);
        VERIFY(doc);
        char* serialized = jc_doc_to_string(doc, 0);
        VERIFY(strcmp(valid_docs[i], serialized) == 0);
        free(serialized);
        jc_free_doc(doc);
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
        JsonDocument_t* doc = jc_doc_from_string(invalid_docs[i]);
        VERIFY(!doc);
    }
})

TEST_CASE(remove_obj, {
    JsonDocument_t* doc = jc_doc_from_string("{\"key\":\"value\"}");
    JsonObject_t* root_obj = jc_doc_get_obj(doc);
    VERIFY(doc && jc_obj_get_string(root_obj, "key"));
    jc_obj_remove(root_obj, "key");
    VERIFY(!jc_obj_get_string(root_obj, "key"));
    jc_free_doc(doc);
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