#include <malloc.h>  // For malloc/free
#include <new>       // For std::size_t
#include "tracy/Tracy.hpp"  // Adjust path if your Tracy include is different

void* operator new(std::size_t size) {
    void* ptr = malloc(size);
    TracyAlloc(ptr, size);
    return ptr;
}

void* operator new[](std::size_t size) {
    void* ptr = malloc(size);
    TracyAlloc(ptr, size);
    return ptr;
}

void operator delete(void* ptr) noexcept {
    TracyFree(ptr);
    free(ptr);
}

void operator delete[](void* ptr) noexcept {
    TracyFree(ptr);
    free(ptr);
}

// Sized deallocation (recommended for modern C++ and fixes the previous warning)
void operator delete(void* ptr, std::size_t size) noexcept {
    TracyFreeS(ptr, size);
    free(ptr);
}

void operator delete[](void* ptr, std::size_t size) noexcept {
    TracyFreeS(ptr, size);
    free(ptr);
}
