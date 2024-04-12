#pragma once
#ifndef __WJCL_HASH_MAP_H__
#define __WJCL_HASH_MAP_H__

#define WJCL_HASH_MAP_DEFAULT_CAPACITY 16
// 1 << 30
#define WJCL_HASH_MAP_MAXIMUM_CAPACITY 1073741824
#define WJCL_HASH_MAP_DEFAULT_LOAD_FACTOR 0.75f
#define WJCL_HASH_MAP_FREE_KEY 0b10
#define WJCL_HASH_MAP_FREE_VALUE 0b01

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "../list/wjcl_linked_list.h"

typedef struct NodeInfo {
    bool (*equalsFunction)(void*, void*);
    uint32_t (*hashFunction)(void*);
    void (*onNodeDelete)(void*, void*);
    uint8_t freeFlag;
} NodeInfo;

typedef struct Map {
    LinkedList* buckets;
    size_t size;
    size_t bukketUsed;
    size_t bucketSize;
    size_t expandSize;
    NodeInfo info;
} Map;

typedef struct MapNode {
    void* key;
    uint32_t keyHash;
    void* value;
} MapNode;
static MapNode emptyMapNode = {0, 0, 0};

#define map_create(info) \
    (Map) { calloc(WJCL_HASH_MAP_DEFAULT_CAPACITY, sizeof(LinkedList)), 0, 0, WJCL_HASH_MAP_DEFAULT_CAPACITY, WJCL_HASH_MAP_DEFAULT_CAPACITY *WJCL_HASH_MAP_DEFAULT_LOAD_FACTOR, info }

/**
 * @brief Foreach all entries in map
 * @param map Hash map
 * @param var Map entry variable name
 * @param each Call at each map entry
 */
#define map_entries(map, var, each) ({                                                \
    for (size_t __entryIndex = 0; __entryIndex < (map)->bucketSize; __entryIndex++) { \
        LinkedListNode* __bucket = ((map)->buckets + __entryIndex)->first;            \
        while (__bucket) {                                                            \
            MapNode* var = (MapNode*)__bucket->value;                                 \
            each;                                                                     \
            __bucket = __bucket->next;                                                \
        }                                                                             \
    }                                                                                 \
})

// Methods
Map* map_new();
/**
 * @brief Put key(pointer) value(pointer) to map
 * @param map Map pointer
 * @param key Key pointer
 * @param value Value pointer (pointer will not be freed when the node is deleted)
 */
MapNode* map_putpp(Map* map, void* key, void* value);
void* map_get(Map* map, void* key);
void map_clear(Map* map);
void map_free(Map* map);

// Implementation
#ifdef WJCL_HASH_MAP_IMPLEMENTATION

Map* map_new(NodeInfo info) {
    Map* map = (Map*)malloc(sizeof(Map));
    map->bucketSize = WJCL_HASH_MAP_DEFAULT_CAPACITY;
    map->buckets = (LinkedList*)calloc(map->bucketSize, sizeof(LinkedList));
    map->size = 0;
    map->bukketUsed = 0;
    map->expandSize = map->bucketSize * WJCL_HASH_MAP_DEFAULT_LOAD_FACTOR;
    map->info = info;
    return map;
}

void expandBucket(Map* map) {
    if (map->bucketSize >= WJCL_HASH_MAP_MAXIMUM_CAPACITY) return;
    size_t newLength = map->bucketSize << 1;
    if (newLength > WJCL_HASH_MAP_MAXIMUM_CAPACITY)
        newLength = WJCL_HASH_MAP_MAXIMUM_CAPACITY;

    // Expand buckets
    map->expandSize = newLength * WJCL_HASH_MAP_DEFAULT_LOAD_FACTOR;
    map->buckets = (LinkedList*)realloc(map->buckets, newLength * sizeof(LinkedList));
    memset(map->buckets + map->bucketSize, 0, (newLength - map->bucketSize) * sizeof(LinkedList));
    map->bucketSize = newLength;

    // Rerange hashtable
    for (size_t i = 0; i < map->bucketSize; i++) {
        LinkedList* bucket = map->buckets + i;
        if (bucket->length == 0) continue;
        LinkedListNode *node = bucket->first, *nextNode;
        while (node) {
            MapNode* mapNode = (MapNode*)node->value;
            // Skip if stay at orignal place
            if ((mapNode->keyHash & (map->bucketSize - 1)) == i) {
                node = node->next;
                continue;
            }
            nextNode = node->next;
            // Move node to destination bucket
            linkedList_removeNode(bucket, node);
            LinkedList* destBucket = map->buckets + (mapNode->keyHash & (map->bucketSize - 1));
            if (destBucket->length == 0) ++map->bukketUsed;
            linkedList_appendNode(destBucket, node);
            node = nextNode;
        }
        if (bucket->length == 0)
            --map->bukketUsed;
    }
}

LinkedListNode* map_getMapNode(Map* map, void* key, uint32_t hashCode) {
    LinkedList* list = map->buckets + (hashCode & (map->bucketSize - 1));
    if (list->length == 0) return NULL;
    // find
    LinkedListNode* node = list->first;
    while (node) {
        MapNode* mapNode = (MapNode*)node->value;
        if (hashCode == mapNode->keyHash && map->info.equalsFunction(key, mapNode->key))
            return node;
        node = node->next;
    }
    return NULL;
}

void* map_get(Map* map, void* key) {
    LinkedListNode* node = map_getMapNode(map, key, map->info.hashFunction(key));
    if (node && node->value)
        return ((MapNode*)node->value)->value;
    return NULL;
}

MapNode* map_putpp(Map* map, void* key, void* value) {
    uint32_t hashCode = map->info.hashFunction(key);
    LinkedListNode* linkedListNode = map_getMapNode(map, key, hashCode);
    MapNode* node;
    // Add new node
    if (linkedListNode && linkedListNode->value) {
        node = (MapNode*)linkedListNode->value;
        if (map->info.onNodeDelete) map->info.onNodeDelete(node->key, node->value);
        if (map->info.freeFlag & WJCL_HASH_MAP_FREE_KEY) free(node->key);
        if (map->info.freeFlag & WJCL_HASH_MAP_FREE_VALUE) free(node->value);
        node->key = key;
        node->value = value;
    }
    // Replace node
    else {
        node = (MapNode*)malloc(sizeof(MapNode));
        node->key = key;
        node->keyHash = hashCode;
        node->value = value;
        if (map->size >= map->expandSize) expandBucket(map);
        LinkedList* destBucket = map->buckets + (hashCode & (map->bucketSize - 1));
        if (destBucket->length == 0) ++(map->bukketUsed);
        linkedList_addPtr(destBucket, node);
        ++(map->size);
    }
    return node;
}

static inline void map_freeMapNode(Map* map, MapNode* entry) {
    if (map->info.onNodeDelete) map->info.onNodeDelete(entry->key, entry->value);
    if (map->info.freeFlag & WJCL_HASH_MAP_FREE_KEY) free(entry->key);
    if (map->info.freeFlag & WJCL_HASH_MAP_FREE_VALUE) free(entry->value);

    free(entry);
}

void map_delete(Map* map, void* key) {
    uint32_t hashCode = map->info.hashFunction(key);
    LinkedListNode* node = map_getMapNode(map, key, hashCode);
    map_freeMapNode(map, (MapNode*)node->value);
    LinkedList* list = map->buckets + (hashCode & (map->bucketSize - 1));
    linkedList_deleteNode(list, node);
}

void map_clear(Map* map) {
    for (size_t i = 0; i < map->bucketSize; i++) {
        LinkedList* bucket = map->buckets + i;
        LinkedListNode* node = bucket->first;
        while (node) {
            LinkedListNode* nextNode = node->next;
            MapNode* entry = (MapNode*)node->value;
            if (map->info.onNodeDelete) map->info.onNodeDelete(entry->key, entry->value);
            if (map->info.freeFlag & WJCL_HASH_MAP_FREE_KEY) free(entry->key);
            if (map->info.freeFlag & WJCL_HASH_MAP_FREE_VALUE) free(entry->value);

            free(entry);
            free(node);
            node = nextNode;
        }
    }
    map->size = 0;
    map->bukketUsed = 0;
    map->bucketSize = WJCL_HASH_MAP_DEFAULT_CAPACITY;
    map->expandSize = map->bucketSize * WJCL_HASH_MAP_DEFAULT_LOAD_FACTOR;
    map->buckets = (LinkedList*)realloc(map->buckets, map->bucketSize * sizeof(LinkedList));
    memset(map->buckets, 0, map->bucketSize * sizeof(LinkedList));
}

void map_free(Map* map) {
    for (size_t i = 0; i < map->bucketSize; i++) {
        LinkedList* bucket = map->buckets + i;
        LinkedListNode* node = bucket->first;
        while (node) {
            LinkedListNode* nextNode = node->next;
            map_freeMapNode(map, (MapNode*)node->value);
            free(node);
            node = nextNode;
        }
    }
    free(map->buckets);
}

void map_printTable(Map* map) {
    printf("\n");
    for (size_t i = 0; i < map->bucketSize; i++) {
        LinkedList* bucket = map->buckets + i;
        printf("%03d|", (int)i);

        LinkedListNode* node = bucket->first;
        while (node) {
            MapNode* mapNode = (MapNode*)node->value;
            printf("%p ", mapNode->value);
            node = node->next;
        }
        printf("\n");
    }
    printf("\n");
}

#endif /* WJCL_HASH_MAP_IMPLEMENTATION */

#endif