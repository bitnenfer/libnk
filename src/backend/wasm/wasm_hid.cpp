#include "../hid_internal.h"
#include "../utils.h"
#include "wasm_structs.h"
#include <nk/hid.h>

void nk::hid::update(NkHID* hid, NkApp* app) {
    hid->keyboard.lastChar = 0;
    memset(hid->keyboard.keysClick, 0, sizeof(hid->keyboard.keysClick));
    memset(hid->mouse.buttonsClick, 0, sizeof(hid->mouse.buttonsClick));
    hid->mouse.wheelDeltaX = 0.0f;
    hid->mouse.wheelDeltaY = 0.0f;

    for (uint32_t index = 0; index < app->eventBuffer.eventNum; ++index) {
        const NkWasmAppEvent& event = app->eventBuffer.events[index];
        switch (event.type) {
        case NkWasmAppEventType::KEYDOWN:
            if (!hid->keyboard.keysDown[event.keyboard.keyCode]) {
                hid->keyboard.keysClick[event.keyboard.keyCode] = true;
            }
            hid->keyboard.keysDown[event.keyboard.keyCode] = true;
            break;
        case NkWasmAppEventType::KEYUP:
            hid->keyboard.keysClick[event.keyboard.keyCode] = false;
            hid->keyboard.keysDown[event.keyboard.keyCode] = false;
            break;
        case NkWasmAppEventType::MOUSEMOVE:
            hid->mouse.positionX = event.mouse.x;
            hid->mouse.positionY = event.mouse.y;
            break;
        case NkWasmAppEventType::MOUSEDOWN:
            if (!hid->mouse.buttonsDown[event.mouse.button]) {
                hid->mouse.buttonsClick[event.mouse.button] = true;
            }
            hid->mouse.buttonsDown[event.mouse.button] = true;
            hid->mouse.positionX = event.mouse.x;
            hid->mouse.positionY = event.mouse.y;
            break;
        case NkWasmAppEventType::MOUSEUP:
            hid->mouse.buttonsDown[event.mouse.button] = false;
            hid->mouse.buttonsClick[event.mouse.button] = false;
            hid->mouse.positionX = event.mouse.x;
            hid->mouse.positionY = event.mouse.y;
            break;
        default:
            NK_LOG("Warning: Invalid event type %u", event.type);
            break;
        }
    }
}

void nk::hid::showCursor(NkHID* hid, bool visible) {
    hid->mouse.visible = visible;
    EM_ASM(document.getElementById('nk-canvas').style.cursor =
               $0 ? 'default' : 'none';
           , visible);
}
