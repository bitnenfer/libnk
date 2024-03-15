#pragma once

#include <nk/hid.h>

struct NkMouse {
    bool buttonsDown[3];
    bool buttonsClick[3];
    float positionX;
    float positionY;
    float wheelDeltaX;
    float wheelDeltaY;
    bool visible;
};

struct NkKeyboard {
    bool keysDown[512];
    bool keysClick[512];
    char lastChar;
};

struct NkHID {
    NkMouse mouse;
    NkKeyboard keyboard;
};

namespace nk {
    namespace hid {

        NkHID* create(NkApp* app);
        bool destroy(NkHID* hid);
        void update(NkHID* hid, NkApp* app);

    } // namespace hid
} // namespace nk
