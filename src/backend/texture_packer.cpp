#include "texture_packer.h"

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
#include "canvas_internal.h"
#include "utils.h"
#include <nk/canvas.h>
#include <stdlib.h>

uint32_t NkTextureAtlasRect::split(uint32_t newWidth, uint32_t newHeight,
                                   NkTextureAtlasRect* outRects) {
    NK_ASSERT(newWidth <= width && newHeight <= height,
              "TextureAtlas: Can't split a rectangle into a larger rectangle.");

    if (width == newWidth && height == newHeight) {
        return 0;
    }

    uint32_t numRects = 0;
    uint32_t oldWidth = width;
    uint32_t oldHeight = height;
    width = newWidth;
    height = newHeight;

    if (newWidth < oldWidth) {
        outRects[numRects++] = {x + newWidth, y, oldWidth - newWidth,
                                newHeight};
    }

    if (newHeight < oldHeight) {
        outRects[numRects++] = {x, y + newHeight, newWidth,
                                oldHeight - newHeight};
    }

    if (newWidth < oldWidth && newHeight < oldHeight) {
        outRects[numRects++] = {x + newWidth, y + newHeight,
                                oldWidth - newWidth, oldHeight - newHeight};
    }

    return numRects;
}

bool NkTextureAtlasRect::canFit(uint32_t rectWidth, uint32_t rectHeight) {
    return rectWidth <= width && rectHeight <= height;
}

void NkTextureAtlasRectArray::init() {
    rects = (NkTextureAtlasRect*)nk::utils::memRealloc(
        nullptr, sizeof(NkTextureAtlasRect) * 16);
    rectNum = 0;
    rectMax = 16;
}

void NkTextureAtlasRectArray::destroy() { nk::utils::memFree(rects); }

void NkTextureAtlasRectArray::add(const NkTextureAtlasRect& rect) {
    if (rectNum + 1 > rectMax) {
        rectMax *= 2;
        NkTextureAtlasRect* newRects =
            (NkTextureAtlasRect*)nk::utils::memRealloc(
                rects, sizeof(NkTextureAtlasRect) * rectMax);
        if (!newRects) {
            NK_PANIC("Error: realloc returned null");
            return;
        }
        rects = newRects;
    }
    rects[rectNum++] = rect;
}

void NkTextureAtlasRectArray::remove(uint32_t index) {
    NK_ASSERT(rectNum > 0 && index < rectNum,
              "Error: Index out of range in texture atlas rect array");
    if (rectNum > 1 && index != rectNum - 1) {
        rects[index] = rects[rectNum - 1];
    }
    rectNum--;
}

void NkTextureAtlasRectArray::reset() { rectNum = 0; }

NkTextureAtlasRect& NkTextureAtlasRectArray::operator[](uint32_t index) {
    NK_ASSERT(index < rectNum,
              "Error: Index out of range in texture atlas rect array");
    return rects[index];
}

void NkTextureAtlas::init(uint32_t width, uint32_t height) {
    new (&images) std::unordered_map<NkImage*, NkTextureAtlasRect>();

    this->width = width;
    this->height = height;
    freeRects.init();
    usedRects.init();
    freeRects.add({0, 0, width, height});
}

template <typename T> void destruct(const T& value) { value.~T(); }

void NkTextureAtlas::destroy() {
    freeRects.destroy();
    usedRects.destroy();
    destruct(images);
}

void NkTextureAtlas::reset() {
    freeRects.reset();
    usedRects.reset();
    freeRects.add({0, 0, width, height});
    images.clear();
}

bool NkTextureAtlas::addRect(uint32_t rectWidth, uint32_t rectHeight,
                             uint32_t id, NkTextureAtlasRect* result) {
    if (rectWidth == 0 || rectHeight == 0)
        return false;
    for (uint32_t index = 0; index < freeRects.rectNum; ++index) {
        if (freeRects[index].canFit(rectWidth, rectHeight)) {
            NkTextureAtlasRect selected = freeRects[index];
            NkTextureAtlasRect split[3];
            uint32_t splitNum = selected.split(rectWidth, rectHeight, split);
            freeRects.remove(index);
            for (uint32_t index = 0; index < splitNum; ++index) {
                freeRects.add(split[index]);
            }
            usedRects.add(selected);
            if (result)
                *result = selected;
            return true;
        }
    }
    return false;
}

const NkTextureAtlasRect& NkTextureAtlas::addImage(NkImage* image) {
#if 0
	auto found = images.find(image);
	if (found != images.end()) {
		return found->second;
	}
	NkTextureAtlasRect rect{};
	uint32_t imageWidth = (uint32_t)nk::img::width(image);
	uint32_t imageHeight = (uint32_t)nk::img::height(image);
	bool result = addRect(imageWidth, imageHeight, 0, &rect);
	NK_ASSERT(result, "TextureAtlas: Failed to add image.");
	rect.uOffset = (float)rect.x / width;
	rect.vOffset = (float)rect.y / height;
	images.insert({ image, rect });
	return rect;
#else
    if (nk::canvas_internal::isImageInTextureAtlas(image)) {
        return nk::canvas_internal::textureRect(image);
    }
    NkTextureAtlasRect rect{};
    uint32_t imageWidth = (uint32_t)nk::img::width(image);
    uint32_t imageHeight = (uint32_t)nk::img::height(image);
    bool result = addRect(imageWidth, imageHeight, 0, &rect);
    NK_ASSERT(result, "TextureAtlas: Failed to add image.");
    rect.uOffset = (float)rect.x / width;
    rect.vOffset = (float)rect.y / height;
    images.insert({image, rect});
    nk::canvas_internal::setTextureAtlasState(image, rect);
    return nk::canvas_internal::textureRect(image);
#endif
}
#endif
