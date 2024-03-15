#pragma once

#include <stddef.h>
#include <stdint.h>

#include <nk/app.h>

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#if NK_PLATFORM_WINDOWS
#define NK_DEBUG_BREAK() __debugbreak()
#else
#include <emscripten.h>
#define NK_DEBUG_BREAK() EM_ASM(debugger)
#endif

#define NK_LOG(fmt, ...) nk::utils::logFmt(fmt "\n", ##__VA_ARGS__)
#define NK_PANIC(fmt, ...)                                                     \
    nk::utils::logFmt("PANIC: " fmt "\n", ##__VA_ARGS__);                      \
    NK_DEBUG_BREAK();                                                          \
    nk::utils::exit(~0);
#define NK_ASSERT(x, fmt, ...)                                                 \
    if (!(x)) {                                                                \
        nk::utils::logFmt("ASSERT: " fmt "\n", ##__VA_ARGS__);                 \
        NK_DEBUG_BREAK();                                                      \
    }
#define NK_ASSERT_EXIT(x, fmt, ...)                                            \
    if (!(x)) {                                                                \
        nk::utils::logFmt("ASSERT: " fmt "\n", ##__VA_ARGS__);                 \
        NK_DEBUG_BREAK();                                                      \
        nk::utils::exit(~0);                                                   \
    }
#define NK_NOT_IMPLEMENTED()                                                   \
    {                                                                          \
        NK_LOG("WARNING: function %s not implemented", __func__);              \
        NK_DEBUG_BREAK();                                                      \
    }

#ifdef NK_SUPERLUMINAL_ENABLED
#include <Superluminal/PerformanceAPI.h>
#define NK_PROFILER_COLOR(r, g, b) PERFORMANCEAPI_MAKE_COLOR(r, g, b)
#define NK_PROFILER_BEGIN_EVENT(name)                                          \
    PerformanceAPI_BeginEvent(name, nullptr, PERFORMANCEAPI_DEFAULT_COLOR)
#define NK_PROFILER_BEGIN_EVENT_COLOR(name, color)                             \
    PerformanceAPI_BeginEvent(name, nullptr, color)
#define NK_PROFILER_END_EVENT() PerformanceAPI_EndEvent()
#define NK_PROFILER_SCOPED_EVENT(name)                                         \
    NkProfilerEvent _profilerEvent##__COUNTER__(name)
#define NK_PROFILER_SCOPED_EVENT_ARGS(name, ...)                               \
    NkProfilerEvent _profilerEvent##__COUNTER__(                               \
        nk::utils::tempString(name, __VA_ARGS__))
#define NK_PROFILER_SCOPED_EVENT_COLOR_ARGS(name, color, ...)                  \
    NkProfilerEvent _profilerEvent##__COUNTER__(                               \
        nk::utils::tempString(name, __VA_ARGS__), color)
struct NkProfilerEvent final {
    NkProfilerEvent(const char* name) { NK_PROFILER_BEGIN_EVENT(name); }
    NkProfilerEvent(const char* name, uint32_t color) {
        NK_PROFILER_BEGIN_EVENT_COLOR(name, color);
    }
    ~NkProfilerEvent() { NK_PROFILER_END_EVENT(); }
};
#else
#define NK_PROFILER_COLOR(r, g, b)
#define NK_PROFILER_BEGIN_EVENT(name)
#define NK_PROFILER_BEGIN_EVENT_COLOR(name, color)
#define NK_PROFILER_END_EVENT()
#define NK_PROFILER_SCOPED_EVENT(name)
#define NK_PROFILER_SCOPED_EVENT_ARGS(name, ...)
#define NK_PROFILER_SCOPED_EVENT_COLOR_ARGS(name, ...)
#endif

namespace nk {

    namespace utils {

        void exit(uint32_t exitCode);
        void logFmt(const char* fmt, ...);
        const char* tempString(const char* fmt, ...);
        size_t bsf(size_t value);
        size_t bsr(size_t value);
        size_t popcnt(size_t value);
        void initMemoryFunctions(NkReallocFunc reallocFunc,
                                 NkFreeFunc freeFunc);
        void* memRealloc(void* ptr, size_t size);
        void memFree(void* ptr);
        void* memZeroAlloc(size_t num, size_t size);
        inline void* offsetPtr(void* Ptr, intptr_t Offset) {
            return (void*)((intptr_t)Ptr + Offset);
        }
        inline void* alignPtr(void* Ptr, size_t Alignment) {
            return (void*)(((uintptr_t)(Ptr) + ((uintptr_t)(Alignment)-1LL)) &
                           ~((uintptr_t)(Alignment)-1LL));
        }
        inline size_t alignSize(size_t Value, size_t Alignment) {
            return ((Value) + ((Alignment)-1LL)) & ~((Alignment)-1LL);
        }
        inline size_t nextPowerOfTwo(size_t value) {
            size_t exp = nk::utils::bsr(value - 1) + 1;
            return (size_t)1 << exp;
        }
        inline bool isPowerOfTwo(size_t value) {
            // Example:
            // a = value		= 0b001000 (8)
            // b = value - 1	= 0b000111 (7)
            // (a & b) == 0
            // if the bitwise AND of the value and value - 1 equals 0
            // then the value is power of two.
            return (value & (value - 1)) == 0;
        }
        inline size_t roundUp(size_t value, size_t roundTo) {
            return ((value + roundTo - 1) / roundTo) * roundTo;
        }
        inline bool isPtrAligned(const void* ptr, size_t alignment) {
            return alignment > 1 ? ((uintptr_t)ptr % alignment) == 0 : true;
        }
        inline int32_t abs(int32_t value) {
            return value < 0 ? ~value + 1 : value;
        }
        inline int64_t abs(int64_t value) {
            return value < 0 ? ~value + 1 : value;
        }
        inline float abs(float value) {
            uint32_t bits =
                *((uint32_t*)&value) & 0x7FFFFFFF; // Just clear the sign bit
            return *((float*)&bits);
        }
        inline double abs(double value) {
            uint64_t bits = *((uint64_t*)&value) &
                            0x7FFFFFFFFFFFFFFFLL; // Just clear the sign bit
            return *((double*)&bits);
        }
        inline size_t ptrDiff(const void* a, const void* b) {
            return (size_t)abs((int64_t)a - (int64_t)b);
        }
        template <typename T> inline T max(T a, T b) { return a > b ? a : b; }
        template <typename T> inline T min(T a, T b) { return a < b ? a : b; }
        template <typename T> inline T clamp(T value, T minValue, T maxValue) {
            return value >= maxValue ? maxValue
                                     : (value <= minValue ? minValue : value);
        }

    } // namespace utils

} // namespace nk
