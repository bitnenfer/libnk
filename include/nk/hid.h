#pragma once

#include <nk/app.h>

struct NkHID;

enum class NkKeyCode : uint32_t {
    ALT = 18,
    DOWN = 40,
    LEFT = 37,
    RIGHT = 39,
    UP = 38,
    BACKSPACE = 8,
    CAPS_LOCK = 20,
    CTRL = 17,
    DEL = 46,
    END = 35,
    ENTER = 13,
    ESC = 27,
    F1 = 112,
    F2 = 113,
    F3 = 114,
    F4 = 115,
    F5 = 116,
    F6 = 117,
    F7 = 118,
    F8 = 119,
    F9 = 120,
    F10 = 121,
    F11 = 122,
    F12 = 123,
    HOME = 36,
    INSERT = 45,
    NUM_LOCK = 144,
    NUMPAD_0 = 96,
    NUMPAD_1 = 97,
    NUMPAD_2 = 98,
    NUMPAD_3 = 99,
    NUMPAD_4 = 100,
    NUMPAD_5 = 101,
    NUMPAD_6 = 102,
    NUMPAD_7 = 103,
    NUMPAD_8 = 104,
    NUMPAD_9 = 105,
    NUMPAD_MINUS = 109,
    NUMPAD_ASTERIKS = 106,
    NUMPAD_DOT = 110,
    NUMPAD_SLASH = 111,
    NUMPAD_SUM = 107,
    PAGE_DOWN = 34,
    PAGE_UP = 33,
    PAUSE = 19,
    PRINT_SCREEN = 44,
    SCROLL_LOCK = 145,
    SHIFT = 16,
    SPACE = 32,
    TAB = 9,
    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,
    NUM_0 = 48,
    NUM_1 = 49,
    NUM_2 = 50,
    NUM_3 = 51,
    NUM_4 = 52,
    NUM_5 = 53,
    NUM_6 = 54,
    NUM_7 = 55,
    NUM_8 = 56,
    NUM_9 = 57,
    QUOTE = 192,
    MINUS = 189,
    COMMA = 188,
    SLASH = 191,
    SEMICOLON = 186,
    LEFT_SQRBRACKET = 219,
    RIGHT_SQRBRACKET = 221,
    BACKSLASH = 220,
    EQUALS = 187
};

enum class NkMouseButton : uint32_t {
    LEFT = 0,
    MIDDLE = 1,
    RIGHT = 2
};

namespace nk {

    namespace hid {

        bool keyDown(NkHID* hid, NkKeyCode keyCode);
        bool keyClick(NkHID* hid, NkKeyCode keyCode);
        char lastChar(NkHID* hid);
        bool mouseDown(NkHID* hid, NkMouseButton button);
        bool mouseClick(NkHID* hid, NkMouseButton button);
        float mouseWheelDeltaX(NkHID* hid);
        float mouseWheelDeltaY(NkHID* hid);
        void showCursor(NkHID* hid, bool visible);
        bool cursorVisible(NkHID* hid);
        float mouseX(NkHID* hid);
        float mouseY(NkHID* hid);

    } // namespace hid

    namespace app {

        NkHID* hid(NkApp* app);

    }

} // namespace nk
