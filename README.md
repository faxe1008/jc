# jc

Simple robust json serialization/deserialization library.

## Example

This example demonstrates the manual construction, serialization and deserialization of a
JSON document:

``` c
JsonObject_t* obj = jc_new_obj();
jc_obj_insert(obj, "key", JC_STRING, "value");

JsonDocument_t* doc = jc_new_doc();
jc_doc_set_obj(doc, obj);

// Indent with 4 spaces, if compact serialization is desired set to 0
char* serialized = jc_doc_to_string(doc, 4);

JsonDocument_t* deserialized_doc = jc_doc_from_string(serialized);
JsonObject_t* root_obj = jc_doc_get_ob(deserialized_doc);
printf("Value of 'key' is: '%s'\n", jc_obj_get_string(root_obj, "key"));

jc_free_doc(doc);
jc_free_doc(deserialized_doc);
free(serialized);
```

The `example` directory also contains a `jpp`, which uses jsonc to read text from stdin or a file and 
pretty prints it.

## About

jsonc is inteded to be used in applications where dynamic memory management is possible.
The implementation is kept as simple as possible while providing efficiency to work on large JSON documents.
