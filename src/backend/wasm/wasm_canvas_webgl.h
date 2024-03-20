#pragma once

#include <nk/canvas.h>

namespace nk {

    namespace canvas {
        NkCanvas* create(NkApp* app, bool allowResize = true);
        bool destroy(NkCanvas* canvas);
    } // namespace canvas

} // namespace nk