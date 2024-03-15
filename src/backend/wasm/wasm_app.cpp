#include "../utils.h"
#include "wasm_canvas_webgpu.h"
#include "wasm_structs.h"

namespace nk {
    namespace hid {

        extern NkHID* create(NkApp* app);
        extern bool destroy(NkHID* hid);
        extern void update(NkHID* hid, NkApp* app);

    } // namespace hid

    namespace canvas {
        extern NkCanvas* create(NkApp* app, bool allowResize);
        extern bool destroy(NkCanvas* canvas);
    } // namespace canvas
} // namespace nk

static EM_BOOL onHTMLKeyboardEvent(int eventType,
                                   const EmscriptenKeyboardEvent* keyEvent,
                                   void* userData) {
    NkApp* app = (NkApp*)userData;
    NkWasmAppEvent event{};
    event.keyboard.keyCode = keyEvent->keyCode;
    if (eventType == EMSCRIPTEN_EVENT_KEYDOWN) {
        event.type = NkWasmAppEventType::KEYDOWN;
    } else if (eventType == EMSCRIPTEN_EVENT_KEYUP) {
        event.type = NkWasmAppEventType::KEYUP;
    }
    if (app->eventBuffer.eventNum + 1 <= NK_APP_WASM_MAX_EVENTS) {
        app->eventBuffer.events[app->eventBuffer.eventNum++] = event;
    }
    return false;
}

static EM_BOOL onHTMLMouseEvent(int eventType,
                                const EmscriptenMouseEvent* mouseEvent,
                                void* userData) {
    NkApp* app = (NkApp*)userData;
    NkWasmAppEvent event{};
    if (eventType == EMSCRIPTEN_EVENT_MOUSEMOVE) {
        event.type = NkWasmAppEventType::MOUSEMOVE;
        event.mouse.x = mouseEvent->targetX;
        event.mouse.y = mouseEvent->targetY;
    } else if (eventType == EMSCRIPTEN_EVENT_MOUSEDOWN) {
        event.type = NkWasmAppEventType::MOUSEDOWN;
        event.mouse.button = mouseEvent->button;
        event.mouse.x = mouseEvent->targetX;
        event.mouse.y = mouseEvent->targetY;
    } else if (eventType == EMSCRIPTEN_EVENT_MOUSEUP) {
        event.type = NkWasmAppEventType::MOUSEUP;
        event.mouse.button = mouseEvent->button;
        event.mouse.x = mouseEvent->targetX;
        event.mouse.y = mouseEvent->targetY;
    }
    if (app->eventBuffer.eventNum + 1 <= NK_APP_WASM_MAX_EVENTS) {
        app->eventBuffer.events[app->eventBuffer.eventNum++] = event;
    }
    return false;
}

static EM_BOOL onHTMLCanvasResize(int eventType,
                                  const EmscriptenUiEvent* uiEvent,
                                  void* userData) {
    if (eventType == EMSCRIPTEN_EVENT_RESIZE) {
        NkApp* app = (NkApp*)userData;
        app->windowWidth = (float)uiEvent->windowInnerWidth;
        app->windowHeight = (float)uiEvent->windowInnerHeight;
        app->canvas->base.resolution[0] = (float)uiEvent->windowInnerWidth;
        app->canvas->base.resolution[1] = (float)uiEvent->windowInnerHeight;
        emscripten_set_canvas_element_size("#nk-canvas",
                                           uiEvent->windowInnerWidth,
                                           uiEvent->windowInnerHeight);
    }
    return true;
}

NkApp* nk::app::create(const NkAppInfo& info) {
    nk::utils::initMemoryFunctions(info.reallocFunc, info.freeFunc);
    NkApp* app = (NkApp*)nk::utils::memZeroAlloc(1, sizeof(NkApp));
    if (!app)
        return nullptr;

    uint32_t windowWidth = info.width;
    uint32_t windowHeight = info.height;

    if (info.fullScreen) {
        windowWidth = (uint32_t)EM_ASM_INT(return window.innerWidth;);
        windowHeight = (uint32_t)EM_ASM_INT(return window.innerHeight;);
    }

    // NOTE: On Web we can't control VSync, so this won't do anything
    // app->vsyncEnabled = info.vsyncEnabled;
    app->windowWidth = windowWidth;
    app->windowHeight = windowHeight;

    EM_ASM(
        const canvas = document.createElement('canvas');
        canvas.id = 'nk-canvas'; canvas.width = $0; canvas.height = $1;
        Module.canvas = canvas; document.body.appendChild(canvas);
        if (UTF8ToString) {
            document.title = UTF8ToString($2);
            document.body.style.backgroundColor = UTF8ToString($3);
        },
        windowWidth, windowHeight, info.title,
        nk::utils::tempString("#%06X", info.backgroundColor));

    app->hid = nk::hid::create(app);
    app->canvas = nk::canvas::create(app, info.allowResize);

    emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, app, 0,
                                    &onHTMLKeyboardEvent);
    emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, app, 0,
                                  &onHTMLKeyboardEvent);
    emscripten_set_mousedown_callback("#nk-canvas", app, 0, &onHTMLMouseEvent);
    emscripten_set_mouseup_callback("#nk-canvas", app, 0, &onHTMLMouseEvent);
    emscripten_set_mousemove_callback("#nk-canvas", app, 0, &onHTMLMouseEvent);

    if (info.fullScreen) {
        emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, app, 0,
                                       &onHTMLCanvasResize);
        EM_ASM(document.body.style.overflow = 'hidden';
               document.getElementsByTagName('html')[0].style.overflow =
                   'hidden';);
    } else {
        EM_ASM(document.body.style.height = '100%';
               document.body.style.display = 'flex';
               document.body.style.justifyContent = 'center';
               document.body.style.alignItems = 'center';
               document.getElementsByTagName('html')[0].style.height = '100%';
               document.getElementsByTagName('html')[0].style.display = 'flex';
               document.getElementsByTagName('html')[0].style.justifyContent =
                   'center';
               document.getElementsByTagName('html')[0].style.alignItems =
                   'center';
               document.getElementById('nk-canvas').style.position =
                   'relative';);
    }

    return app;
}

bool nk::app::destroy(NkApp* app) {
    if (app) {
        nk::hid::destroy(app->hid);
        nk::utils::memFree(app);
        return true;
    }
    return false;
}

void nk::app::update(NkApp* app) {
    EM_ASM(if (window.frameTime) { window.frameTime.tick(); });
    nk::hid::update(app->hid, app);
    app->eventBuffer.eventNum = 0;
}

bool nk::app::shouldQuit(const NkApp* app) { return app->shouldQuit; }

uint32_t nk::app::windowWidth(const NkApp* app) { return app->windowWidth; }

uint32_t nk::app::windowHeight(const NkApp* app) { return app->windowHeight; }

bool nk::app::shouldResize(const NkApp* app, uint32_t* newWidth,
                           uint32_t* newHeight) {
    return false;
}

NkHID* nk::app::hid(NkApp* app) { return app->hid; }

void nk::app::quit(NkApp* app) { app->shouldQuit = true; }

NkCanvas* nk::app::canvas(NkApp* app) { return app->canvas; }
