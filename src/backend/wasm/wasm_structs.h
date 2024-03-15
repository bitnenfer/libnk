#pragma once

#include "wasm_common.h"
#include <nk/canvas.h>
#include <nk/hid.h>

#define NK_APP_WASM_MAX_EVENTS 256

enum class NkWasmAppEventType {
    KEYDOWN,
    KEYUP,
    MOUSEMOVE,
    MOUSEDOWN,
    MOUSEUP
};

struct NkWasmAppMouseEventData {
    int32_t button;
    uint32_t x;
    uint32_t y;
};

struct NkWasmAppKeyboardEventData {
    int32_t keyCode;
};

struct NkWasmAppEvent {
    NkWasmAppEventType type;
    union {
        NkWasmAppMouseEventData mouse;
        NkWasmAppKeyboardEventData keyboard;
    };
};

struct NkAppEventBuffer {
    NkWasmAppEvent events[NK_APP_WASM_MAX_EVENTS];
    uint32_t eventNum;
};

struct NkApp {
    NkAppEventBuffer eventBuffer;
    NkCanvas* canvas;
    NkHID* hid;
    uint32_t windowWidth;
    uint32_t windowHeight;
    bool shouldQuit;
};
