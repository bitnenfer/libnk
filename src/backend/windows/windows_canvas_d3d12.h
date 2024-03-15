#pragma once

#include "../canvas_internal.h"
#include "../utils.h"
#include "windows_structs.h"
#include <assert.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <shlobj.h>
#include <string>
#include <strsafe.h>

#define D3D_PANIC(fmt, ...)                                                    \
    NK_LOG(fmt, __VA_ARGS__);                                                  \
    __debugbreak();                                                            \
    ExitProcess(-1);
#define D3D_ASSERT(x, ...)                                                     \
    if ((x) != S_OK) {                                                         \
        D3D_PANIC(__VA_ARGS__);                                                \
    }
#define D3D_RELEASE(obj)                                                       \
    {                                                                          \
        if ((obj)) {                                                           \
            (obj)->Release();                                                  \
            obj = nullptr;                                                     \
        }                                                                      \
    }

#define NK_CANVAS_D3D12_MAX_BARRIERS                  32
#define NK_CANVAS_D3D12_MAX_FRAMES                    2
#define NK_CANVAS_D3D12_MAX_COMMAND_LISTS             20
#define NK_CANVAS_D3D12_MAX_DESCRIPTOR_NUM            (1 << 16)
#define NK_CANVAS_D3D12_MAX_UPLOAD_BUFFERS            512
#define NK_CANVAS_D3D12_MAX_BACKBUFFERS               2
#define NK_CANVAS_D3D12_MAX_SRV_DESCRIPTORS_PER_FRAME (1 << 16)

enum class NkBufferType {
    INDEX_BUFFER,
    VERTEX_BUFFER,
    CONSTANT_BUFFER,
    UNORDERED_BUFFER,
    UPLOAD_BUFFER,
    SHADER_RESOURCE_BUFFER
};

struct NkD3D12Instance {
    ID3D12Device5* device;
    ID3D12Debug3* debugInterface;
    IDXGIFactory1* factory;
    IDXGIAdapter1* adapter;
    uint32_t refCount;
};

struct NkD3D12DescriptorTable {

    inline D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle(uint64_t index) {
        D3D12_GPU_DESCRIPTOR_HANDLE handle = gpuBaseHandle;
        handle.ptr += (index * handleSize);
        return handle;
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle(uint64_t index) {
        D3D12_CPU_DESCRIPTOR_HANDLE handle = cpuBaseHandle;
        handle.ptr += (index * handleSize);
        return handle;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE gpuBaseHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuBaseHandle;
    uint32_t handleSize;
};

struct NkD3D12DescriptorAllocator {

    void init(uint32_t descriptorNum, D3D12_DESCRIPTOR_HEAP_TYPE type);
    void destroy();
    void reset();
    NkD3D12DescriptorTable allocate(uint32_t descriptorNum);

    ID3D12DescriptorHeap* descriptorHeap;
    D3D12_GPU_DESCRIPTOR_HANDLE gpuBaseHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE cpuBaseHandle;
    uint32_t descriptorHandleSize;
    uint32_t descriptorAllocated;
};

struct NkD3D12Fence {

    void init(ID3D12Device* device, uint64_t initialValue = 0);
    void destroy();
    void waitForValue(uint64_t waitValue);

    ID3D12Fence* fence;
    HANDLE event;
};

struct NkImageArray {

    void init();
    void destroy();
    void reset();
    void add(NkImage* image);

    NkImage** images;
    uint32_t imageNum;
    uint32_t imageMax;
};

struct NkD3D12Resource {
    ID3D12Resource* resource;
    D3D12_RESOURCE_STATES state;
    D3D12_RESOURCE_DESC desc;
};

struct NkD3D12DynamicBuffer {
    NkD3D12Resource resource;
    ID3D12Resource* uploadBuffer;
};

struct NkD3D12CommandList {
    ID3D12GraphicsCommandList* commandList;
    ID3D12CommandAllocator* commandAllocator;
};

struct NkCanvas {
    NkCanvasBase base;
    NkApp* app;
    ID3D12DescriptorHeap* rtvDescriptorHeap;
    NkD3D12DescriptorAllocator descriptorAllocators[NK_CANVAS_MAX_FRAMES];
    NkD3D12CommandList commandLists[NK_CANVAS_MAX_FRAMES];
    ID3D12CommandQueue* commandQueue;
    IDXGISwapChain3* swapChain;
    ID3D12RootSignature* spriteRootSignature;
    ID3D12PipelineState* spritePSO;
    ID3D12Resource* backbuffers[NK_CANVAS_D3D12_MAX_FRAMES];
    float clearColor[4];
    NkImageArray imageUploads[NK_CANVAS_MAX_FRAMES];
    NkImageArray imagesToDestroy[NK_CANVAS_MAX_FRAMES];
    bool allowResize;
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    NkImage* images[NK_CANVAS_D3D12_MAX_SRV_DESCRIPTORS_PER_FRAME];
    uint32_t imageNum;
#endif
    NkImage* renderTarget;
};

#define NK_IMAGE_BIT_UPLOADED                  0b00001
#define NK_IMAGE_BIT_SAVED                     0b00010
#define NK_IMAGE_BIT_TEXTURE_ATLAS             0b00100
#define NK_IMAGE_BIT_TEXTURE_ATLAS_RESIDENT    0b01000
#define NK_IMAGE_BIT_BOUND_TO_DESCRIPTOR_TABLE 0b10000

struct NkImage {
    NkD3D12DynamicBuffer buffer;
    float width;
    float height;
    uint32_t state; // bit 0 = uploaded, bit 1 = saved, bit 2 = added to texture
                    // atlas
    void* cpuData;
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    NkTextureAtlasRect rect;
#endif
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    uint32_t textureId;
#endif
};

namespace nk {

    namespace canvas {
        extern NkCanvas* create(NkApp* app, bool allowResize = true);
        extern bool destroy(NkCanvas* canvas);
    } // namespace canvas

    namespace d3d12 {

        NkD3D12Instance* createInstance(NkApp* app);
        void destroyInstance(NkD3D12Instance* instance);
        NkD3D12Instance* instance();
        ID3D12Resource* createNativeBuffer(size_t bufferSize,
                                           NkBufferType bufferType,
                                           const wchar_t* name,
                                           D3D12_RESOURCE_STATES* outInitState);
        NkD3D12Resource* createBuffer(size_t bufferSize,
                                      NkBufferType bufferType,
                                      const wchar_t* name = L"Buffer");
        NkD3D12DynamicBuffer*
        createDynamicBuffer(size_t bufferSize, NkBufferType bufferType,
                            const wchar_t* name = L"DynamicBuffer");
        NkImage* createImage(NkCanvas* canvas, uint32_t width, uint32_t height,
                             NkImageFormat format, const void* pixels);
        NkImage* createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                         uint32_t height);
        void destroyImage(NkImage* image);
        size_t getDXGIFormatBits(DXGI_FORMAT format);
        size_t getDXGIFormatBytes(DXGI_FORMAT format);
        void drawFrame(NkCanvas* canvas, uint64_t currentFrameIndex);
        void processImage(NkCanvas* canvas, NkImage* image);
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
        void updateTextureAtlas(NkTextureAtlas& textureAtlas,
                                ID3D12GraphicsCommandList* commandList);
#endif

    } // namespace d3d12

} // namespace nk
