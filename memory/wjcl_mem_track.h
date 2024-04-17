#pragma once
#ifndef __WJCL_MEM_TRACK_H__
#define __WJCL_MEM_TRACK_H__

#include <stdio.h>
#include <stdlib.h>

#define MEM_TRACK
#ifdef MEM_TRACK
void* (*untrackedMalloc)(size_t __size) = malloc;
void* (*untrackedCalloc)(size_t __nmemb, size_t __size) = calloc;
void* (*untrackedRealloc)(void* __ptr, size_t __size) = realloc;
void (*untrackedFree)(void* __ptr) = free;

#include "../map/wjcl_hash_map.h"
#define malloc(size) newMem(__FILE__, __LINE__, size, untrackedMalloc(size))
#define calloc(count, size) newMem(__FILE__, __LINE__, count* size, untrackedCalloc(count, size))
#define realloc(ptr, size) reMem(__FILE__, __LINE__, count* size, untrackedRealloc(ptr, size))
#define free(ptr) freeMem(ptr);

typedef struct MemInfo {
    char* lineName;
    size_t size;
} MemInfo;

bool memPtrEquals(void* a, void* b) {
    return *(size_t*)a == *(size_t*)b;
}

uint32_t memPtrHash(void* a) {
    return (uint32_t)(*(size_t*)a) + (uint32_t)((*(size_t*)a) >> 32);
}

void memInfoFree(void* a) {
    MemInfo* info = (MemInfo*)a;
}

const NodeInfo memTableInfo = {
    .equalsFunction = memPtrEquals,
    .hashFunction = memPtrHash,
    .onNodeDelete = NULL,
    .freeFlag = WJCL_HASH_MAP_FREE_KEY | WJCL_HASH_MAP_FREE_VALUE,
};

Map memTable = map_create(memTableInfo);

void* newMem(const char* fileName, int line, size_t size, void* ptr) {
    // char cache[128];
    // size_t len = sprintf(cache, "%s:%d", fileName, line);
    MemInfo* info = (MemInfo*)untrackedMalloc(sizeof(MemInfo));
    info->lineName = NULL;
    info->size = size;

    size_t* key = (size_t*)untrackedMalloc(sizeof(size_t));
    *key = (size_t)ptr;
    map_putpp(&memTable, key, info);
    return ptr;
}

void* reMem(const char* fileName, int line, size_t size, void* ptr) {
    if (size == 0) {
        map_delete(&memTable, &ptr);
        return ptr;
    }
    MemInfo* info = map_get(&memTable, &ptr);
    if (!info)
        newMem(fileName, line, size, ptr);
    else
        info->size = size;
    return ptr;
}

void freeMem(void* ptr) {
    untrackedFree(ptr);
    map_delete(&memTable, &ptr);
}

void memTrackResult() {
    printf("Not freed ptr count: %d\n", memTable.size);
    size_t wastedSize = 0;
    map_entries(&memTable, entries, {
        MemInfo* info = (MemInfo*)entries->value;
        wastedSize += info->size;
    });
    printf("Wasted size: %llu(bytes)\n", wastedSize);
}

#define memTrackResult() memTrackResult()

#endif

#endif