#pragma once
#ifndef __WJCL_LINKED_LIST_H__
#define __WJCL_LINKED_LIST_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "../memory/wjcl_mem_track_util.h"

typedef struct LinkedListNode {
    struct LinkedListNode* previous;
    struct LinkedListNode* next;
    void* value;
    // Flag for freeing value
    uint8_t freeFlag;
} LinkedListNode;

typedef struct LinkedList {
    size_t length;
    LinkedListNode* first;
    LinkedListNode* last;
} LinkedList;

#define linkedList_create() \
    { 0, NULL, NULL }

/**
 * @brief Add a value to linked list
 * @param list LinkedList pointer
 * @param value Value
 */
#define linkedList_add(list, value) *(typeof(value)*)linkedList_addp(list, 1, __malloc(sizeof(value))) = value

/**
 * @brief Add a pointer to linked list
 * @param list LinkedList pointer
 * @param value Value pointer (pointer will not be freed when the node is deleted)
 */
#define linkedList_addPtr(list, value) linkedList_addp(list, 0, value)

#define linkedList_get(list, type, index) *(type*)linkedList_getNode(list, index)->value

#define linkedList_getPtr(list, index) linkedList_getNode(list, index)->value

#define linkedList_foreach(list, variable, callBack)    \
    {                                                   \
        LinkedListNode* node = (list)->first;           \
        while (node) {                                  \
            variable = *(typeof(variable)*)node->value; \
            callBack;                                   \
            node = (node)->next;                        \
        }                                               \
    }

#define linkedList_foreachPtr(list, variable, callBack) \
    {                                                   \
        LinkedListNode* node = (list)->first;           \
        while (node) {                                  \
            variable = node->value;                     \
            callBack;                                   \
            node = (node)->next;                        \
        }                                               \
    }

#define linkedList_free(list) linkedList_freeA(list, NULL)

// Method
void linkedList_appendNode(LinkedList* list, LinkedListNode* node);
void linkedList_deleteNode(LinkedList* list, LinkedListNode* node);
void linkedList_removeNode(LinkedList* list, LinkedListNode* node);
void linkedList_freeA(LinkedList* list, void (*freeValue)(void* value));

LinkedList* linkedList_new();
void* linkedList_addp(LinkedList* list, uint8_t freeFlag, void* value);

// Implementation
// #define WJCL_LINKED_LIST_IMPLEMENTATION
#ifdef WJCL_LINKED_LIST_IMPLEMENTATION

LinkedList* linkedList_new() {
    LinkedList* list = (LinkedList*)__malloc(sizeof(LinkedList));
    list->first = list->last = NULL;
    list->length = 0;
    return list;
}

void* linkedList_addp(LinkedList* list, uint8_t freeFlag, void* value) {
    LinkedListNode* node = (LinkedListNode*)__malloc(sizeof(LinkedListNode));
    node->freeFlag = freeFlag;
    node->next = NULL;
    if (list->first == NULL) {
        node->previous = NULL;
        list->last = list->first = node;
    } else {
        node->previous = list->last;
        list->last->next = node;
        list->last = node;
    }
    ++(list->length);
    return node->value = value;
}

void linkedList_appendNode(LinkedList* list, LinkedListNode* node) {
    node->next = NULL;
    if (list->first == NULL) {
        node->previous = NULL;
        list->last = list->first = node;
    } else {
        node->previous = list->last;
        list->last->next = node;
        list->last = node;
    }
    ++(list->length);
}

LinkedListNode* linkedList_getNode(LinkedList* list, size_t index) {
    if (index > list->length) return NULL;
    LinkedListNode* node;
    if (index > list->length / 2) {
        node = list->last;
        for (size_t i = list->length - 1; i > index; --i)
            node = node->previous;
    } else {
        node = list->first;
        for (size_t i = 0; i < index; ++i)
            node = node->next;
    }
    return node;
}

void linkedList_removeNode(LinkedList* list, LinkedListNode* node) {
    if (node->previous)
        node->previous->next = node->next;
    else
        list->first = node->next;
    if (node->next)
        node->next->previous = node->previous;
    else
        list->last = node->previous;
    --(list->length);
}

/**
 * @brief Free node value if flag set, and free node
 * @param list
 * @param node
 */
void linkedList_deleteNode(LinkedList* list, LinkedListNode* node) {
    if (node->previous)
        node->previous->next = node->next;
    else
        list->first = node->next;
    if (node->next)
        node->next->previous = node->previous;
    else
        list->last = node->previous;
    --(list->length);
    if (node->freeFlag)
        __free(node->value);
    __free(node);
}

void linkedList_freeA(LinkedList* list, void (*freeValue)(void* value)) {
    LinkedListNode* node = list->first;
    while (node) {
        LinkedListNode* nextNode = node->next;
        if (freeValue)
            freeValue(node->value);
        if (node->freeFlag)
            __free(node->value);
        __free(node);
        node = nextNode;
    }
    list->first = NULL;
    list->last = NULL;
    list->length = 0;
}

#endif /* WJCL_LINKED_LIST_TYPE_IMPLEMENTATION */

#endif