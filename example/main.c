#include <jsonc.h>
#include <stddef.h>

int main(int argc, char** argv)
{
    JsonValue_t* value = jsonc_new_value(STRING, "General Kenobi");

    JsonObject_t* obj = jsonc_new_obj();
    jsonc_obj_set(obj, "Hello There", value);

    JsonValue_t* new_val = jsonc_new_value(NULL_LITERAL, NULL);
    jsonc_obj_set(obj, "Hello There", new_val);

    JsonDocument_t* doc = jsonc_new_doc();
    jsonc_doc_set_obj(doc, obj);

    jsonc_free_doc(doc);
}