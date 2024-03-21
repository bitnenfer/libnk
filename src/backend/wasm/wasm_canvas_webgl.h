#pragma once

#include "../canvas_internal.h"
#include "../utils.h"
#include "wasm_structs.h"
#include <GLES2/gl2.h>
#include <emscripten/html5_webgl.h>

#define NK_IMAGE_BIT_UPLOADED               0b0001
#define NK_IMAGE_BIT_SAVED                  0b0010
#define NK_IMAGE_BIT_TEXTURE_ATLAS          0b0100
#define NK_IMAGE_BIT_TEXTURE_ATLAS_RESIDENT 0b1000

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
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    uint32_t state;
    NkTextureAtlasRect rect;
#endif
};

struct NkCanvas {
    NkCanvasBase base;
    NkApp* app;
    GLuint spriteProgram;
    GLuint spriteVertShader;
    GLuint spriteFragShader;
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    GLuint textureAtlasProgram;
    GLuint textureAtlasVertShader;
    GLuint textureAtlasFragShader;
    GLuint textureAtlasVB;
#endif
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
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
        void updateTextureAtlas(NkCanvas* canvas, NkTextureAtlas& textureAtlas);
#endif
    } // namespace webgl

} // namespace nk