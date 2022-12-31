# jsonc

Simple robust json serialization/deserialization library.

## Example

This example demonstrates the manual construction, serialization and deserialization of a
JSON document:

``` c
JsonObject_t* obj = jsonc_new_obj();
jsonc_obj_insert(obj, "key", JSONC_STRING, "value");

JsonDocument_t* doc = jsonc_new_doc();
jsonc_doc_set_obj(doc, obj);

// Indent with 4 spaces, if compact serialization is desired set to 0
char* serialized = jsonc_doc_to_string(doc, 4);

JsonDocument_t* deserialized_doc = jsonc_doc_from_string(serialized);
printf("Value of 'key' is: '%s'\n", jsonc_obj_get_string(doc->object, "key"));

jsonc_free_doc(doc);
jsonc_free_doc(deserialized_doc);
free(serialized);
```

The `example` directory also contains a `jpp`, which uses jsonc to read text from stdin or a file and 
pretty prints it.

## About

jsonc is inteded to be used in applications where dynamic memory management is possible.
The implementation is kept as simple as possible while providing efficiency to work on large JSON documents.
It currently does not support unicode characters properly.