#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef _DEBUG
#define NK_RELEASE 0
#define NK_DEBUG   1
#else
#define NK_RELEASE 1
#define NK_DEBUG   0
#endif

#define NK_COLOR_UINT(color)                                                   \
    (((color) & 0xff) << 24) | ((((color) >> 8) & 0xff) << 16) |               \
        ((((color) >> 16) & 0xff) << 8) | ((color) >> 24)
#define NK_COLOR_RGBA_UINT(r, g, b, a)                                         \
    ((uint32_t)(r)) | ((uint32_t)(g) << 8) | ((uint32_t)(b) << 16) |           \
        ((uint32_t)(a) << 24)
#define NK_COLOR_RGB_UINT(r, g, b) NK_COLOR_RGBA_UINT(r, g, b, 0xff)
#define NK_COLOR_RGBA_FLOAT(r, g, b, a)                                        \
    NK_COLOR_RGBA_UINT((uint8_t)((r) * 255.0f), (uint8_t)((g) * 255.0f),       \
                       (uint8_t)((b) * 255.0f), (uint8_t)((a) * 255.0f))
#define NK_COLOR_RGB_FLOAT(r, g, b) NK_COLOR_RGBA_FLOAT(r, g, b, 1.0f)

typedef void* (*NkReallocFunc)(void* ptr, size_t size);
typedef void (*NkFreeFunc)(void* ptr);

struct NkApp;

struct NkAppInfo {
    uint32_t width;
    uint32_t height;
    const char* title;
    bool vsyncEnabled;
    uint32_t backgroundColor;
    bool allowResize;
    bool fullScreen;
    NkReallocFunc reallocFunc;
    NkFreeFunc freeFunc;
};

namespace nk {

    namespace app {

        NkApp* create(const NkAppInfo& info);
        bool destroy(NkApp* app);
        void update(NkApp* app);
        bool shouldQuit(const NkApp* app);
        uint32_t windowWidth(const NkApp* app);
        uint32_t windowHeight(const NkApp* app);
        bool shouldResize(const NkApp* app, uint32_t* newWidth,
                          uint32_t* newHeight);
        void quit(NkApp* app);

    } // namespace app

} // namespace nk
