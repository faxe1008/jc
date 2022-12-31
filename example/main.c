#include <assert.h>
#include <jsonc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string_builder.h>

static bool read_doc_from_file(StringBuilder_t*, FILE*);

#ifndef READ_BUFFER_CAP
#    define READ_BUFFER_CAP 1024
#endif

int main(int argc, char** argv)
{
    StringBuilder_t builder = { 0 };
    builder_resize(&builder, READ_BUFFER_CAP);

    bool read_result = true;
    if (argc == 1) {
        read_result = read_doc_from_file(&builder, stdin);
    } else if (argc == 2) {
        if (strcmp(argv[1], "-") == 0) {
            read_result = read_doc_from_file(&builder, stdin);
        } else {
            FILE* f = fopen(argv[1], "rb");
            read_result = read_doc_from_file(&builder, f);
        }
    }

    if (!read_result) {
        printf("Could not read input\n");
        return 1;
    }

    JsonDocument_t* doc = jsonc_doc_from_string(builder.buffer);
    if (!doc) {
        printf("Error parsing document\n");
        return 1;
    }

    char* prettified = jsonc_doc_to_string(doc, 4);
    if (!prettified) {
        printf("Error pretty printing document\n");
        jsonc_free_doc(doc);
        return 1;
    }

    printf("%s\n", prettified);
    jsonc_free_doc(doc);
    free(builder.buffer);
    free(prettified);
}

bool read_doc_from_file(StringBuilder_t* builder, FILE* file)
{
    if (!file) {
        perror("Error");
        return false;
    }
    bool result = true;
    int ch = EOF;
    while ((ch = fgetc(file)) != EOF) {
        if (ch > 256 || !builder_append_ch(builder, (char)ch)) {
            result = false;
            goto EXIT;
        }
    }
EXIT:
    fclose(file);
    return result;
}