#pragma once
#ifndef __WJCL_LIST_TYPE_H__
#define __WJCL_LIST_TYPE_H__

#define WJCL_LIST_TYPE_INIT_LENGTH 3
// #define WJCL_LIST_PRINT_ERROR

#include <stdlib.h>
#include <stdio.h>

typedef struct ListT {
    void* i;
    size_t length;
    size_t _len;
    size_t itemSize;
} ListT;

#define listT_create(type) \
    { (void*)malloc(WJCL_LIST_TYPE_INIT_LENGTH * sizeof(type)), 0, WJCL_LIST_TYPE_INIT_LENGTH, sizeof(type) }

#define listT_new(type) listT_newp(sizeof(type))

#define listT_add(list, value) ({                                               \
    if ((list)->length == (list)->_len) listT_extend(list);                     \
    *(typeof(value)*)((list)->i + (list)->length++ * (list)->itemSize) = value; \
})

#define listT_getPtr(list, index) ((list)->i + index * (list)->itemSize)

#define listT_get(list, type, index) *(type*)listT_getPtr(list, index)

#define list_foreach(list, type, varName, callBack) ({                   \
    for (size_t __index = 0; __index < (list)->length; ++__index) {      \
        type varName = *(type*)((list)->i + __index * (list)->itemSize); \
        callBack;                                                        \
    }                                                                    \
})

#define list_foreachPtr(list, type, varName, callBack) ({           \
    for (size_t __index = 0; __index < (list)->length; ++__index) { \
        type varName = (list)->i + __index * (list)->itemSize;      \
        callBack;                                                   \
    }                                                               \
})

#define listT_clear(list) listT_clearA(list, NULL)
#define listT_free(list) listT_freeA(list, NULL)

// Method
void listT_clearA(ListT* list, void (*freeValue)(void* value));
void listT_freeA(ListT* list, void (*freeValue)(void* value));

// Implementation
#ifdef WJCL_LIST_TYPE_IMPLEMENTATION

void listT_extend(ListT* list) {
    void* newArray = realloc(list->i, (list->_len *= 1.5) * list->itemSize);
#ifdef WJCL_LIST_PRINT_ERROR
    if (newArray == NULL)
        fprintf(stderr, "[ListT ERROR] List realloc failed!");
    else
#endif
        list->i = newArray;
}

ListT* listT_newp(size_t itemSize) {
    ListT* list = (ListT*)malloc(sizeof(ListT));
    list->_len = WJCL_LIST_TYPE_INIT_LENGTH;
    list->length = 0;
    list->itemSize = itemSize;
    void* array = malloc(list->_len * list->itemSize);
#ifdef WJCL_LIST_PRINT_ERROR
    if (array == NULL)
        fprintf(stderr, "[ListT ERROR] List realloc failed!");
    else
#endif
        list->i = array;
    return list;
}

void listT_clearA(ListT* list, void (*freeValue)(void* value)) {
    if (freeValue)
        for (size_t i = 0; i < list->length; ++i)
            freeValue((void*)(list->i + i * list->itemSize));

    list->_len = WJCL_LIST_TYPE_INIT_LENGTH;
    list->length = 0;
    void* newArray = realloc(list->i, WJCL_LIST_TYPE_INIT_LENGTH * list->itemSize);
#ifdef WJCL_LIST_PRINT_ERROR
    if (newArray == NULL)
        fprintf(stderr, "[ListT ERROR] List realloc failed!");
    else
#endif
        list->i = newArray;
}

void listT_freeA(ListT* list, void (*freeValue)(void* value)) {
    if (freeValue)
        for (size_t i = 0; i < list->length; ++i)
            freeValue(*(void**)(list->i + i * list->itemSize));
    free(list->i);
}

#endif /* WJCL_LIST_TYPE_IMPLEMENTATION */

#endif