#ifndef JSONC_OLH_MAP__
#define JSONC_OLH_MAP__

#include <stdbool.h>
#include <stddef.h>
typedef enum {
    EMPTY,
    OCCUPIED,
    DELETED
} BucketState;

typedef struct BucketEntry_t {
    BucketState state;
    char* key;
    void* value;
    struct BucketEntry_t* previous;
    struct BucketEntry_t* next;
} BucketEntry_t;

typedef void (*olh_map_value_free)(void*);

typedef struct {
    size_t capacity;
    size_t size;
    size_t deleted_count;
    BucketEntry_t* buckets;
    BucketEntry_t* head;
    BucketEntry_t* tail;
    olh_map_value_free value_free_func;
} OrderedLinkedHashMap_t;

bool olh_map_rehash(OrderedLinkedHashMap_t* map, size_t capacity);
bool olh_map_set(OrderedLinkedHashMap_t* map, const char* key, void* data);
void* olh_map_get(const OrderedLinkedHashMap_t* map, const char* key);
bool olh_map_remove(OrderedLinkedHashMap_t* map, const char* key);
void olh_map_free(OrderedLinkedHashMap_t* map);

#endif