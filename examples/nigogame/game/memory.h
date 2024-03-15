#pragma once

#include <assert.h>
#include <new>
#include <stdlib.h>

template <typename T> struct NcObjectAllocator {

    struct NcObjectNode {
        T object;
        NcObjectNode* next;
    };

    void init(uint32_t objectCapacity) {
        nodes = (NcObjectNode*)malloc(sizeof(NcObjectNode) * objectCapacity);
        freeList = nullptr;
        allocated = 0;
        capacity = objectCapacity;
    }

    void destroy() {
        ::free(nodes);
        freeList = nullptr;
        allocated = 0;
        capacity = 0;
    }

    T* allocate(uint32_t count = 1) {
        if (freeList != nullptr) {
            NcObjectNode* node = freeList;
            freeList = node->next;
            return &node->object;
        }
        if (allocated + 1 <= capacity) {
            return &nodes[allocated++].object;
        }
        assert(0); // out of memory
        return nullptr;
    }

    template <typename... TArgs> T* create(TArgs&&... args) {
        return new (allocate()) T(args...);
    }

    void free(T* ptr) {
        NcObjectNode* node = reinterpret_cast<NcObjectNode*>(ptr);
        if (node->next == nullptr) {
            node->next = freeList;
            freeList = node;
        }
    }

    void reset() {
        freeList = nullptr;
        allocated = 0;
    }

private:
    NcObjectNode* nodes;
    NcObjectNode* freeList;
    uint32_t allocated;
    uint32_t capacity;
};
