#include "../utils.h"
#include "windows_structs.h"
#include <nk/app.h>

namespace nk {
    namespace hid {

        extern NkHID* create(NkApp* app);
        extern bool destroy(NkHID* hid);
        extern void update(NkHID* hid, NkApp* app);

    } // namespace hid

    namespace canvas {
        extern NkCanvas* create(NkApp* app, bool allowResize = true);
        extern bool destroy(NkCanvas* canvas);
    } // namespace canvas
} // namespace nk

NkAppResizeInfo internalResizeInfo = {0, 0, false};

NkApp* nk::app::create(const NkAppInfo& info) {
    nk::utils::initMemoryFunctions(info.reallocFunc, info.freeFunc);
    NkApp* app = (NkApp*)nk::utils::memZeroAlloc(1, sizeof(NkApp));
    if (!app)
        return nullptr;

    if (info.allowResize && info.fullScreen) {
        NK_LOG("Warning: Window resize and full screen is not supported. Full "
               "screen will take priority");
    }

    uint32_t windowWidth = info.width;
    uint32_t windowHeight = info.height;
    if (info.fullScreen) {
        windowWidth = GetSystemMetrics(SM_CXSCREEN);
        windowHeight = GetSystemMetrics(SM_CXSCREEN);
    }
    app->vsyncEnabled = info.vsyncEnabled;
    app->windowWidth = windowWidth;
    app->windowHeight = windowHeight;
    internalResizeInfo.width = windowWidth;
    internalResizeInfo.height = windowHeight;
    internalResizeInfo.shouldResize = false;
    internalResizeInfo.oldWidth = app->windowWidth;
    internalResizeInfo.oldHeight = app->windowHeight;

    /* Create Window */
    WNDCLASS windowClass = {
        .style = 0,
        .lpfnWndProc = [](HWND windowHandle, UINT message, WPARAM wParam,
                          LPARAM lParam) -> LRESULT {
            switch (message) {
            case WM_SIZE:
                internalResizeInfo.width = LOWORD(lParam);
                internalResizeInfo.height = HIWORD(lParam);
                internalResizeInfo.shouldResize = true;
                break;
            case WM_CONTEXTMENU:
                break;
            case WM_ENTERSIZEMOVE:
                break;
            case WM_EXITSIZEMOVE:
                break;
            case WM_CLOSE:
                DestroyWindow(windowHandle);
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(windowHandle, message, wParam, lParam);
            }
            return S_OK;
        },
        .cbClsExtra = 0,
        .cbWndExtra = 0,
        .hInstance = GetModuleHandle(nullptr),
        .hIcon = LoadIcon(nullptr, IDI_APPLICATION),
        .hCursor = LoadCursorA(nullptr, IDC_ARROW),
        .hbrBackground = (HBRUSH)(COLOR_WINDOW + 1),
        .lpszMenuName = nullptr,
        .lpszClassName = "WindowClass"};
    RegisterClassA(&windowClass);

    DWORD windowStyle = WS_VISIBLE | WS_SYSMENU | WS_CAPTION | WS_BORDER;

    if (info.allowResize) {
        windowStyle |= WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    }

    if (info.fullScreen) {
        windowStyle = WS_POPUP | WS_VISIBLE;
    }

    RECT windowRect = {0, 0, (LONG)windowWidth, (LONG)windowHeight};
    AdjustWindowRect(&windowRect, windowStyle, false);
    int32_t adjustedWidth = windowRect.right - windowRect.left;
    int32_t adjustedHeight = windowRect.bottom - windowRect.top;

    app->windowHandle = CreateWindowExA(
        WS_EX_LEFT, "WindowClass", info.title, windowStyle, CW_USEDEFAULT,
        CW_USEDEFAULT, adjustedWidth, adjustedHeight, nullptr, nullptr,
        GetModuleHandle(nullptr), nullptr);

    app->shouldQuit = false;
    app->windowed = true;
    app->hid = nk::hid::create(app);
    app->canvas = nk::canvas::create(app, info.allowResize);
    return app;
}

NkCanvas* nk::app::canvas(NkApp* app) { return app->canvas; }

bool nk::app::destroy(NkApp* app) {
    nk::hid::destroy(app->hid);
    nk::canvas::destroy(app->canvas);
    CloseWindow(app->windowHandle);
    DestroyWindow(app->windowHandle);
    app->shouldQuit = true;

    if (app != nullptr) {
        nk::utils::memFree(app);
    }
    return false;
}

void nk::app::update(NkApp* app) {
    MSG message;
    if (internalResizeInfo.shouldResize) {
        internalResizeInfo.oldWidth = app->windowWidth;
        internalResizeInfo.oldHeight = app->windowHeight;
        app->windowWidth = internalResizeInfo.width;
        app->windowHeight = internalResizeInfo.height;
    }
    internalResizeInfo.shouldResize = false;
    app->eventBuffer.eventNum = 0;
    while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE)) {
        switch (message.message) {
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
        case WM_CHAR:
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            if (app->eventBuffer.eventNum < NK_MAX_APP_EVENTS) {
                app->eventBuffer.events[app->eventBuffer.eventNum++] = message;
            } else {
                NK_LOG("Warning: Reached max event count");
            }
            break;
        case WM_QUIT:
            app->shouldQuit = true;
            return;
        default:
            DispatchMessageA(&message);
            break;
        }
    }
    nk::hid::update(app->hid, app);
}

bool nk::app::shouldQuit(const NkApp* app) { return app->shouldQuit; }

NkHID* nk::app::hid(NkApp* app) { return app->hid; }

uint32_t nk::app::windowWidth(const NkApp* app) { return app->windowWidth; }

uint32_t nk::app::windowHeight(const NkApp* app) { return app->windowHeight; }

bool nk::app::shouldResize(const NkApp* app, uint32_t* newWidth,
                           uint32_t* newHeight) {
    if (internalResizeInfo.shouldResize) {
        if (newWidth)
            *newWidth = internalResizeInfo.width;
        if (newHeight)
            *newHeight = internalResizeInfo.height;
        return true;
    }
    return false;
}

void nk::app::quit(NkApp* app) { app->shouldQuit = true; }
