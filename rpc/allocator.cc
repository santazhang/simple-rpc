#ifdef USE_TCMALLOC

#include "gperftools/tcmalloc.h"
#define malloc_fn(size) tc_malloc(size)
#define free_fn(ptr) tc_free(ptr)

#include "allocator.h"

// scalar, throwing new and matching delete
void* operator new(std::size_t size) noexcept(false) {
    void* ptr = malloc_fn(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void operator delete(void* ptr) noexcept(true) {
    free_fn(ptr);
}

// scalar, nothrow new and matching delete
void* operator new(std::size_t size, const std::nothrow_t&) noexcept(true) {
    try {
        return operator new(size);
    } catch (...) {
        // ignore
    }
    return nullptr;
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept(true) {
    free_fn(ptr);
}

// array throwing new and matching delete[]
void* operator new[](std::size_t size) noexcept(false) {
    void* ptr = malloc_fn(size);
    if (ptr == nullptr) {
        throw std::bad_alloc();
    }
    return ptr;
}

void operator delete[](void* ptr) noexcept(true) {
    free_fn(ptr);
}

// array, nothrow new and matching delete[]
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept(true) {
    try {
        return operator new[](size);
    } catch (...) {
        // ignore
    }
    return nullptr;
}

void operator delete[](void* ptr, const std::nothrow_t&) noexcept(true) {
    free_fn(ptr);
}

#endif // USE_TCMALLOC
