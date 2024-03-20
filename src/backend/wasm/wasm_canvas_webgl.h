#pragma once

#include "../canvas_internal.h"
#include "../utils.h"
#include "wasm_structs.h"
#include <GLES2/gl2.h>
#include <emscripten/html5_webgl.h>

struct NkWebGLInstance {
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;
    int32_t refCount;
};

struct NkImage {
    GLuint texture;
    GLuint framebuffer;
    GLuint renderbuffer;
    float width;
    float height;
};

struct NkCanvas {
    NkCanvasBase base;
    NkApp* app;
    GLuint spriteProgram;
    GLuint spriteVertShader;
    GLuint spriteFragShader;
    NkImage* renderTarget;
    float clearColor[4];
    bool allowResize;
};

namespace nk {

    namespace canvas {
        NkCanvas* create(NkApp* app, bool allowResize = true);
        bool destroy(NkCanvas* canvas);
    } // namespace canvas

    namespace webgl {
        NkWebGLInstance* createInstance();
        void destroyInstance(NkWebGLInstance* instance);
        NkWebGLInstance* instance();
        void drawFrame(NkCanvas* canvas, uint64_t currentFrameIndex);
    } // namespace webgl

} // namespace nk