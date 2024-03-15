#pragma once

#include "../canvas_internal.h"
#include "../utils.h"
#include "wasm_structs.h"
#include <emscripten/html5_webgpu.h>

struct NkWebGPUInstance {
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    uint32_t refCount;
};

struct NkWebGPUBuffer {
    WGPUBuffer buffer;
};

#define NK_IMAGE_BIT_UPLOADED               0b0001
#define NK_IMAGE_BIT_SAVED                  0b0010
#define NK_IMAGE_BIT_TEXTURE_ATLAS          0b0100
#define NK_IMAGE_BIT_TEXTURE_ATLAS_RESIDENT 0b1000

struct NkImage {
    WGPUTexture texture;
    WGPUTextureView textureView;
    WGPUBindGroup bindGroup;
    float width;
    float height;
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    uint32_t state;
    NkTextureAtlasRect rect;
#endif
};

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
struct NkTextureAtlasResource {
    WGPUTexture texture;
    WGPUTextureView textureView;
    WGPUBindGroup bindGroup;
};
#endif

struct NkCanvas {
    NkCanvasBase base;
    NkApp* app;
    WGPUSurface canvasSurface;
    WGPUSwapChain swapChain;
    WGPUTextureFormat surfaceFormat;
    WGPURenderPipeline spritePSO;
    WGPUShaderModule spriteShaderModule;
    WGPUPipelineLayout spritePipelineLayout;
    WGPUBindGroupLayout spriteBindGroupLayout[2];
    WGPUBuffer resolutionBuffer;
    WGPUBindGroup staticBindGroup;
    WGPUSampler pointSampler;
    float clearColor[4];
    bool allowResize;
    NkImage* renderTarget;
};

namespace nk {

    namespace canvas {
        NkCanvas* create(NkApp* app, bool allowResize = true);
        bool destroy(NkCanvas* canvas);
    } // namespace canvas

    namespace webgpu {
        NkWebGPUInstance* createInstance();
        void destroyInstance(NkWebGPUInstance* instance);
        NkWebGPUInstance* instance();
        void drawFrame(NkCanvas* canvas, uint64_t currentFrameIndex);
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
        void updateTextureAtlas(NkTextureAtlas& textureAtlas,
                                WGPUCommandEncoder commandEncoder);
#endif
    } // namespace webgpu
} // namespace nk
