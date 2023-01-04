#include <assert.h>
#include <olh_map.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static inline uint32_t jenkins_hash(const char* data)
{
    size_t len = strlen(data); // TODO: maybe put into bucket entry
    uint32_t hash = 0;
    for (size_t i = 0; i < len; i++) {
        hash += ((uint8_t*)data)[i];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

static inline uint32_t double_hash(uint32_t hash)
{
    const uint32_t magic = 0xBA5EDB01;
    if (hash == magic)
        return 0u;
    if (hash == 0u)
        hash = magic;

    hash ^= hash << 13;
    hash ^= hash >> 17;
    hash ^= hash << 5;
    return hash;
}

bool olh_map_rehash(OrderedLinkedHashMap_t* map, size_t capacity)
{
    capacity = (4 > capacity ? 4 : capacity);

    BucketEntry_t* old_buckets = map->buckets;
    BucketEntry_t* old_head = map->head;

    map->buckets = calloc(1, sizeof(BucketEntry_t) * capacity);
    if (!map->buckets) {
        map->buckets = old_buckets;
        return false;
    }
    map->capacity = capacity;
    map->size = 0;
    map->deleted_count = 0;
    map->head = NULL;
    map->tail = NULL;

    if (!old_buckets)
        return true;

    while (old_head) {
        if (old_head->state == OCCUPIED) {
            olh_map_set(map, old_head->key, old_head->value);
        }
        BucketEntry_t* next = old_head->next;
        free(old_head->key);
        old_head = next;
    }
    free(old_buckets);

    return true;
}

static inline bool should_grow(const OrderedLinkedHashMap_t* map)
{
    return (map->size + map->deleted_count + 1) >= map->capacity;
}

static BucketEntry_t* lookup_bucket_for_read(const OrderedLinkedHashMap_t* map, const char* key)
{
    if (map->size == 0)
        return NULL;

    uint32_t hash = jenkins_hash(key);
    for (;;) {
        BucketEntry_t* candidate = &map->buckets[hash % map->capacity];
        if (candidate->state == OCCUPIED && strcmp(candidate->key, key) == 0)
            return candidate;
        if (candidate->state != OCCUPIED && candidate->state != DELETED)
            return NULL;

        hash = double_hash(hash);
    }
}

static BucketEntry_t* lookup_bucket_for_write(OrderedLinkedHashMap_t* map, const char* key)
{
    if (should_grow(map))
        olh_map_rehash(map, map->capacity * 2);

    uint32_t hash = jenkins_hash(key);
    BucketEntry_t* first_empty_bucket = NULL;
    for (;;) {
        BucketEntry_t* candidate = &map->buckets[hash % map->capacity];

        if (candidate->state == OCCUPIED && strcmp(candidate->key, key) == 0)
            return candidate;

        if (candidate->state != OCCUPIED) {
            if (!first_empty_bucket)
                first_empty_bucket = candidate;

            if (candidate->state != DELETED)
                return first_empty_bucket;
        }

        hash = double_hash(hash);
    }
}

bool olh_map_set(OrderedLinkedHashMap_t* map, const char* key, void* data)
{
    assert(map && key && data && map->value_free_func);
    BucketEntry_t* bucket = lookup_bucket_for_write(map, key);
    if (!bucket)
        return false;
    if (bucket->state == OCCUPIED)
        goto SET_VALUE;

    bucket->key = strdup(key);
    if (!bucket->key)
        return false;

    map->size++;

    if (bucket->state == DELETED)
        map->deleted_count--;

    if (!map->head) {
        map->head = bucket;
    } else {
        bucket->previous = map->tail;
        map->tail->next = bucket;
    }
    map->tail = bucket;
    bucket->state = OCCUPIED;

SET_VALUE:
    if (bucket->value)
        map->value_free_func(bucket->value);
    bucket->value = data;
    return true;
}

void* olh_map_get(const OrderedLinkedHashMap_t* map, const char* key)
{
    BucketEntry_t* bucket = lookup_bucket_for_read(map, key);
    if (!bucket)
        return NULL;
    return bucket->value;
}

bool olh_map_remove(OrderedLinkedHashMap_t* map, const char* key)
{
    assert(map && key && map->value_free_func);
    BucketEntry_t* bucket = lookup_bucket_for_read(map, key);
    if (bucket && bucket->state == OCCUPIED) {
        if (bucket->previous)
            bucket->previous->next = bucket->next;
        else
            map->head = bucket->next;

        if (bucket->next)
            bucket->next->previous = bucket->previous;
        else
            map->tail = bucket->previous;

        if (bucket->key) {
            free(bucket->key);
            bucket->key = NULL;
        }
        if (bucket->value) {
            map->value_free_func(bucket->value);
            bucket->value = NULL;
        }
        bucket->state = DELETED;
        map->size--;
        map->deleted_count++;
        if (map->deleted_count >= map->size && should_grow(map))
            olh_map_rehash(map, map->capacity);
        return true;
    }
    return false;
}

void olh_map_free(OrderedLinkedHashMap_t* map)
{
    BucketEntry_t* current = map->head;
    while (current) {
        BucketEntry_t* next = current->next;
        if (current->key)
            free(current->key);
        map->value_free_func(current->value);
        current = next;
    }
    if (map->buckets)
        free(map->buckets);
}