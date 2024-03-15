#pragma once

#include "windows_common.h"
#include <nk/canvas.h>
#include <nk/hid.h>

#define NK_MAX_APP_EVENTS 256

struct NkAppEventBuffer {
    MSG events[NK_MAX_APP_EVENTS];
    uint32_t eventNum;
};

struct NkApp {
    NkAppEventBuffer eventBuffer;
    NkCanvas* canvas;
    NkHID* hid;
    HWND windowHandle;
    uint32_t windowWidth;
    uint32_t windowHeight;
    bool shouldQuit;
    bool windowed;
    bool vsyncEnabled;
};

struct NkAppResizeInfo {
    uint32_t oldWidth;
    uint32_t oldHeight;
    uint32_t width;
    uint32_t height;
    bool shouldResize;
};
