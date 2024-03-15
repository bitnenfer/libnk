#pragma once

#include <stddef.h>
#include <stdint.h>
#include <unordered_map>

#ifdef NK_CANVAS_TEXTURE_ATLAS_ENABLED
#undef NK_CANVAS_TEXTURE_ATLAS_ENABLED
#define NK_CANVAS_TEXTURE_ATLAS_ENABLED 1
#endif

#define NK_CANVAS_TEXTURE_ATLAS_WIDTH  4096
#define NK_CANVAS_TEXTURE_ATLAS_HEIGHT 4096

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
struct NkImage;

struct NkTextureAtlasRect {

    uint32_t split(uint32_t newWidth, uint32_t newHeight,
                   NkTextureAtlasRect* outRects);
    bool canFit(uint32_t rectWidth, uint32_t rectHeight);

    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    float uOffset;
    float vOffset;
};

struct NkTextureAtlasRectArray {

    void init();
    void destroy();

    void add(const NkTextureAtlasRect& rect);
    void remove(uint32_t index);
    void reset();
    NkTextureAtlasRect& operator[](uint32_t index);

    NkTextureAtlasRect* rects;
    uint32_t rectNum;
    uint32_t rectMax;
};

struct NkTextureAtlas {

    void init(uint32_t width, uint32_t height);
    void destroy();
    void reset();
    bool addRect(uint32_t rectWidth, uint32_t rectHeight, uint32_t id = 0,
                 NkTextureAtlasRect* result = nullptr);
    const NkTextureAtlasRect& addImage(NkImage* image);

    std::unordered_map<NkImage*, NkTextureAtlasRect> images;
    NkTextureAtlasRectArray freeRects;
    NkTextureAtlasRectArray usedRects;
    uint32_t width;
    uint32_t height;
    void* gpuTexture;
};
#endif
