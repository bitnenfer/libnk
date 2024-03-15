#include "../utils.h"
#include "windows_common.h"

#define NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE  4096
#define NK_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT 4

static NkReallocFunc globalRealloc = nullptr;
static NkFreeFunc globalFree = nullptr;

void nk::utils::exit(uint32_t exitCode) { ExitProcess(exitCode); }

void nk::utils::logFmt(const char* fmt, ...) {
    static char bufferLarge[NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE *
                            NK_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT] = {};
    static uint32_t bufferIndex = 0;
    va_list args;
    va_start(args, fmt);
    char* buffer =
        &bufferLarge[bufferIndex * NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE];
    vsprintf_s(buffer, NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE, fmt, args);
    bufferIndex = (bufferIndex + 1) % NK_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT;
    va_end(args);
    OutputDebugStringA(buffer);
    printf("%s", buffer);
}

const char* nk::utils::tempString(const char* fmt, ...) {
    static char bufferLarge[NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE *
                            NK_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT] = {};
    static uint32_t bufferIndex = 0;
    va_list args;
    va_start(args, fmt);
    char* buffer =
        &bufferLarge[bufferIndex * NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE];
    vsprintf_s(buffer, NK_UTILS_WINDOWS_LOG_MAX_BUFFER_SIZE, fmt, args);
    bufferIndex = (bufferIndex + 1) % NK_UTILS_WINDOWS_LOG_MAX_BUFFER_COUNT;
    va_end(args);
    return buffer;
}

size_t nk::utils::bsf(size_t value) {
    if (value == 0)
        return 64;
    unsigned long index = 0;
    _BitScanForward64(&index, value);
    return index;
}

size_t nk::utils::bsr(size_t value) {
    if (value == 0)
        return 64;
    unsigned long index = 0;
    _BitScanReverse64(&index, value);
    return index;
}

size_t nk::utils::popcnt(size_t value) { return __popcnt64(value); }

void nk::utils::initMemoryFunctions(NkReallocFunc reallocFunc,
                                    NkFreeFunc freeFunc) {
    globalRealloc = reallocFunc;
    globalFree = freeFunc;
    if (globalRealloc == nullptr || globalFree == nullptr) {
        NK_LOG("Using stdlib realloc and free");
        globalRealloc = &realloc;
        globalFree = &free;
    }
}

void* nk::utils::memRealloc(void* ptr, size_t size) {
    NK_ASSERT_EXIT(globalRealloc,
                   "Error: Memory allocation functions not setup. "
                   "nk::utils::initMemoryFunctions must be called.");
    return globalRealloc(ptr, size);
}

void nk::utils::memFree(void* ptr) {
    NK_ASSERT_EXIT(globalFree,
                   "Error: Memory allocation functions not setup. "
                   "nk::utils::initMemoryFunctions must be called.");
    globalFree(ptr);
}

void* nk::utils::memZeroAlloc(size_t num, size_t size) {
    void* ptr = memRealloc(nullptr, size * num);
    memset(ptr, 0, size * num);
    return ptr;
}
