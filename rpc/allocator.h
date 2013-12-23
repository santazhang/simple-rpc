#pragma once

#ifdef USE_TCMALLOC

#include <new>

// scalar, throwing new and matching delete
void* operator new(std::size_t size) noexcept(false);
void operator delete(void* ptr) noexcept(true);

// scalar, nothrow new and matching delete
void* operator new(std::size_t size, const std::nothrow_t&) noexcept(true);
void operator delete(void* ptr, const std::nothrow_t&) noexcept(true);

// array throwing new and matching delete[]
void* operator new[](std::size_t size) noexcept(false);
void operator delete[](void* ptr) noexcept(true);

// array, nothrow new and matching delete[]
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept(true);

void operator delete[](void* ptr, const std::nothrow_t&) noexcept(true);

#endif // USE_TCMALLOC
