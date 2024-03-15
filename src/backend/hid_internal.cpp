#include "hid_internal.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

NkHID* nk::hid::create(NkApp* app) {
    NkHID* hid = (NkHID*)nk::utils::memZeroAlloc(1, sizeof(NkHID));
    if (!hid)
        return nullptr;
    memset(hid, 0, sizeof(NkHID));
    hid->mouse.positionX = 0.0f;
    hid->mouse.positionY = 0.0f;
    hid->mouse.wheelDeltaX = 0.0f;
    hid->mouse.wheelDeltaY = 0.0f;

    nk::hid::showCursor(hid, false);
    nk::hid::showCursor(hid, true);

    return hid;
}

bool nk::hid::destroy(NkHID* hid) {
    if (hid) {
        nk::utils::memFree(hid);
        return true;
    }
    return false;
}

bool nk::hid::keyDown(NkHID* hid, NkKeyCode keyCode) {
    return hid->keyboard.keysDown[(uint32_t)keyCode];
}

bool nk::hid::keyClick(NkHID* hid, NkKeyCode keyCode) {
    return hid->keyboard.keysClick[(uint32_t)keyCode];
}

char nk::hid::lastChar(NkHID* hid) { return hid->keyboard.lastChar; }

bool nk::hid::mouseDown(NkHID* hid, NkMouseButton button) {
    return hid->mouse.buttonsDown[(uint32_t)button];
}

bool nk::hid::mouseClick(NkHID* hid, NkMouseButton button) {
    return hid->mouse.buttonsClick[(uint32_t)button];
}

float nk::hid::mouseWheelDeltaX(NkHID* hid) { return hid->mouse.wheelDeltaX; }

float nk::hid::mouseWheelDeltaY(NkHID* hid) { return hid->mouse.wheelDeltaY; }

bool nk::hid::cursorVisible(NkHID* hid) { return hid->mouse.visible; }

float nk::hid::mouseX(NkHID* hid) { return hid->mouse.positionX; }

float nk::hid::mouseY(NkHID* hid) { return hid->mouse.positionY; }
