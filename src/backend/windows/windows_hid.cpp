#include "../hid_internal.h"
#include "windows_common.h"
#include "windows_structs.h"
#include <nk/hid.h>

void nk::hid::update(NkHID* hid, NkApp* app) {

    hid->keyboard.lastChar = 0;
    memset(hid->keyboard.keysClick, 0, sizeof(hid->keyboard.keysClick));
    memset(hid->mouse.buttonsClick, 0, sizeof(hid->mouse.buttonsClick));
    hid->mouse.wheelDeltaX = 0.0f;
    hid->mouse.wheelDeltaY = 0.0f;

    for (uint32_t index = 0; index < app->eventBuffer.eventNum; ++index) {
        MSG event = app->eventBuffer.events[index];
        switch (event.message) {
        case WM_MOUSEWHEEL:
            hid->mouse.wheelDeltaY =
                (float)GET_WHEEL_DELTA_WPARAM(event.wParam) /
                (float)WHEEL_DELTA;
            break;
        case WM_MOUSEHWHEEL:
            hid->mouse.wheelDeltaX =
                -(float)GET_WHEEL_DELTA_WPARAM(event.wParam) /
                (float)WHEEL_DELTA;
            break;
        case WM_CHAR:
            hid->keyboard.lastChar = (char)event.wParam;
            break;
        case WM_KEYDOWN:
            if (!hid->keyboard.keysDown[(uint8_t)event.wParam]) {
                hid->keyboard.keysClick[(uint8_t)event.wParam] = true;
            }
            hid->keyboard.keysDown[(uint8_t)event.wParam] = true;
            TranslateMessage(&event);
            break;
        case WM_KEYUP:
            hid->keyboard.keysDown[(uint8_t)event.wParam] = false;
            hid->keyboard.keysClick[(uint8_t)event.wParam] = false;
            break;
        case WM_MOUSEMOVE:
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        case WM_LBUTTONDOWN:
            if (!hid->mouse.buttonsDown[(uint32_t)NkMouseButton::LEFT]) {
                hid->mouse.buttonsClick[(uint32_t)NkMouseButton::LEFT] = true;
            }
            hid->mouse.buttonsDown[(uint32_t)NkMouseButton::LEFT] = true;
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        case WM_LBUTTONUP:
            hid->mouse.buttonsDown[(uint32_t)NkMouseButton::LEFT] = false;
            hid->mouse.buttonsClick[(uint32_t)NkMouseButton::LEFT] = false;
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        case WM_RBUTTONDOWN:
            if (!hid->mouse.buttonsDown[(uint32_t)NkMouseButton::RIGHT]) {
                hid->mouse.buttonsClick[(uint32_t)NkMouseButton::RIGHT] = true;
            }
            hid->mouse.buttonsDown[(uint32_t)NkMouseButton::RIGHT] = true;
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        case WM_RBUTTONUP:
            hid->mouse.buttonsDown[(uint32_t)NkMouseButton::RIGHT] = false;
            hid->mouse.buttonsClick[(uint32_t)NkMouseButton::RIGHT] = false;
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        case WM_MBUTTONDOWN:
            if (!hid->mouse.buttonsDown[(uint32_t)NkMouseButton::MIDDLE]) {
                hid->mouse.buttonsClick[(uint32_t)NkMouseButton::MIDDLE] = true;
            }
            hid->mouse.buttonsDown[(uint32_t)NkMouseButton::MIDDLE] = true;
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        case WM_MBUTTONUP:
            hid->mouse.buttonsDown[(uint32_t)NkMouseButton::MIDDLE] = false;
            hid->mouse.buttonsClick[(uint32_t)NkMouseButton::MIDDLE] = false;
            hid->mouse.positionX = (float)GET_X_LPARAM(event.lParam);
            hid->mouse.positionY = (float)GET_Y_LPARAM(event.lParam);
            break;
        default:
            break;
        }
    }
}

void nk::hid::showCursor(NkHID* hid, bool visible) {
    hid->mouse.visible = visible;
    ShowCursor(visible);
}
