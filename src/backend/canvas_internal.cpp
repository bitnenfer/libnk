#include "canvas_internal.h"
#include <stdlib.h>
#include <string.h>

void NkCanvasVertexBufferAllocator::init() {
    vertexBuffers = (NkCanvasVertexBuffer*)nk::utils::memZeroAlloc(
        NK_CANVAS_MAX_VERTEX_BUFFERS, sizeof(NkCanvasVertexBuffer));
    vertexBufferNum = 0;
    vertexBufferMax = 0;
}

void NkCanvasVertexBufferAllocator::destroy() {
    for (uint32_t index = 0; index < vertexBufferMax; ++index) {
        nk::utils::memFree(vertexBuffers[index].vertices);
        nk::canvas_internal::destroyVertexBuffer(&vertexBuffers[index]);
    }
    nk::utils::memFree(vertexBuffers);
}

void NkCanvasVertexBufferAllocator::reset() { vertexBufferNum = 0; }

NkCanvasVertexBuffer* NkCanvasVertexBufferAllocator::allocate() {
    NK_ASSERT(vertexBufferNum + 1 < NK_CANVAS_MAX_VERTEX_BUFFERS,
              "Error: Exceeded the limit of %u vertex buffers per frame.",
              NK_CANVAS_MAX_VERTEX_BUFFERS);
    NkCanvasVertexBuffer& vertexBuffer = vertexBuffers[vertexBufferNum++];
    if (!vertexBuffer.gpuVertexBuffer) {
        vertexBuffer.vertices = (NkCanvasVertex*)nk::utils::memRealloc(
            nullptr,
            NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER * sizeof(NkCanvasVertex));
        nk::canvas_internal::initVertexBuffer(&vertexBuffer);
    }
    vertexBuffer.vertexCount = 0;
    if (vertexBufferNum > vertexBufferMax) {
        vertexBufferMax = vertexBufferNum;
    }
    return &vertexBuffer;
}

void NkCanvasDrawBatchInternalArray::init() {
    drawBatches = (NkCanvasDrawBatchInternal*)nk::utils::memZeroAlloc(
        NK_CANVAS_MAX_BATCHES, sizeof(NkCanvasDrawBatchInternal));
    drawBatchNum = 0;
}

void NkCanvasDrawBatchInternalArray::destroy() {
    nk::utils::memFree(drawBatches);
}

void NkCanvasDrawBatchInternalArray::reset() { drawBatchNum = 0; }

void NkCanvasDrawBatchInternalArray::add(
    const NkCanvasDrawBatchInternal& drawBatch) {
    NK_ASSERT(drawBatchNum + 1 < NK_CANVAS_MAX_BATCHES,
              "Error: Exceeded the limit of %u draw batches per frame.",
              NK_CANVAS_MAX_BATCHES);
    drawBatches[drawBatchNum++] = drawBatch;
}

NkCanvasDrawBatchInternal* NkCanvasDrawBatchInternalArray::last() {
    if (drawBatchNum > 0) {
        return &drawBatches[drawBatchNum - 1];
    }
    return nullptr;
}

void NkCanvasBase::init(NkCanvas* canvas, float width, float height) {
    for (uint32_t index = 0; index < NK_CANVAS_MAX_FRAMES; ++index) {
        vertexBufferAllocator[index].init();
        drawBatchArray[index].init();
        nk::canvas_internal::initFrameSyncPoint(&gpuFrameSyncPoint[index]);
    }
    indices = (uint32_t*)nk::utils::memZeroAlloc(NK_CANVAS_MAX_INDICES,
                                                 NK_CANVAS_INDEX_SIZE);

    if (!indices) {
        NK_PANIC("Error: Failed to allocate resource for canvas.");
        return;
    }

    for (uint32_t index = 0, vertex = 0; index < NK_CANVAS_MAX_INDICES;
         index += NK_CANVAS_INDICES_PER_QUAD) {
        uint32_t v0 = vertex;
        uint32_t v1 = vertex + 1;
        uint32_t v2 = vertex + 2;
        uint32_t v3 = vertex + 3;
        indices[index + 0] = v0;
        indices[index + 1] = v1;
        indices[index + 2] = v2;
        indices[index + 3] = v0;
        indices[index + 4] = v2;
        indices[index + 5] = v3;
        vertex += 4;
    }
    nk::canvas_internal::initIndexBuffer(&gpuIndexBuffer);
    whiteImage = nullptr;
    resolution[0] = width;
    resolution[1] = height;
    currVertexBuffer = nullptr;
    lastFrameIndex = 0;
    currentFrame = 1;
    matrixStack.init();
    currDrawBatch = nullptr;
    this->canvas = canvas;
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    frameTextureAtlas.init(NK_CANVAS_TEXTURE_ATLAS_WIDTH,
                           NK_CANVAS_TEXTURE_ATLAS_HEIGHT);
#endif
}

void NkCanvasBase::destroy(NkCanvas* canvas) {
    for (uint32_t index = 0; index < NK_CANVAS_MAX_FRAMES; ++index) {
        nk::canvas_internal::waitFrameSyncPoint(gpuFrameSyncPoint[index],
                                                gpuFrameWaitValue[index]);
        vertexBufferAllocator[index].destroy();
        drawBatchArray[index].destroy();
        nk::canvas_internal::destroyFrameSyncPoint(&gpuFrameSyncPoint[index]);
    }

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    frameTextureAtlas.destroy();
    nk::canvas_internal::destroyTextureAtlasResource(frameTextureAtlas);
#endif
    nk::canvas_internal::destroyIndexBuffer(&gpuIndexBuffer);
    nk::canvas::destroyImage(canvas, whiteImage);
    nk::utils::memFree(indices);
}

void NkCanvasBase::waitCurrentFrame() {
    nk::canvas_internal::waitFrameSyncPoint(
        gpuFrameSyncPoint[currentFrameIndex],
        gpuFrameWaitValue[currentFrameIndex]);
}

void NkCanvasBase::signalCurrentFrame() {
    nk::canvas_internal::signalFrameSyncPoint(
        canvas, gpuFrameSyncPoint[currentFrameIndex], currentFrame);
    gpuFrameWaitValue[currentFrameIndex] = currentFrame;
}

NkCanvasVertexBuffer* NkCanvasBase::allocateVertexBuffer() {
    NkCanvasVertexBuffer* vertexBuffer =
        vertexBufferAllocator[currentFrameIndex].allocate();
    vertexBuffer->vertexCount = 0;
    return vertexBuffer;
}

const void* NkCanvasBase::indexBufferData() const {
    return (const void*)indices;
}

void NkCanvasBase::pushQuad(const NkCanvasVertex* vertices, NkImage* image) {
    NK_ASSERT(image != nullptr, "Error: Passing null image to canvas");
    if (!currVertexBuffer ||
        currVertexBuffer->vertexCount + NK_CANVAS_VERTICES_PER_QUAD >
            NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER) {
        bool shouldAddNewBatch = currVertexBuffer != nullptr;
        currVertexBuffer = allocateVertexBuffer();
        if (shouldAddNewBatch) {
            currDrawBatch = nextDrawBatch(image);
        }
    }

    if (!currDrawBatch
#if !NK_CANVAS_TEXTURE_ATLAS_ENABLED && !NK_CANVAS_BINDLESS_RESOURCE_ENABLED
        || currDrawBatch->image != image
#endif
    ) {
        currDrawBatch = nextDrawBatch(image);
    }

    NkCanvasVertex* writeVertices = allocVertices();
    memcpy(writeVertices, vertices,
           NK_CANVAS_VERTICES_PER_QUAD * sizeof(NkCanvasVertex));
    NkCanvasMatrix& matrix = currentMatrix();
    matrix.multiply(writeVertices[0].position[0], writeVertices[0].position[1]);
    matrix.multiply(writeVertices[1].position[0], writeVertices[1].position[1]);
    matrix.multiply(writeVertices[2].position[0], writeVertices[2].position[1]);
    matrix.multiply(writeVertices[3].position[0], writeVertices[3].position[1]);

#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    uint32_t textureId = nk::canvas_internal::addImageToTable(canvas, image);
    writeVertices[0].textureId = textureId;
    writeVertices[1].textureId = textureId;
    writeVertices[2].textureId = textureId;
    writeVertices[3].textureId = textureId;
#endif

    currDrawBatch->count += NK_CANVAS_INDICES_PER_QUAD;
}

NkCanvasDrawBatchInternal*
NkCanvasBase::addDrawBatch(const NkCanvasDrawBatchInternal& drawBatch) {
    drawBatchArray[currentFrameIndex].add(drawBatch);
    return drawBatchArray[currentFrameIndex].last();
}

NkCanvasDrawBatchInternal* NkCanvasBase::nextDrawBatch(NkImage* image) {
    NkCanvasDrawBatchInternal drawBatch{};
    drawBatch.image = image;
    drawBatch.buffer = currVertexBuffer;
    drawBatch.bufferOffset = currVertexBuffer->vertexCount;
    drawBatch.count = 0;

    return addDrawBatch(drawBatch);
}

NkCanvasVertex* NkCanvasBase::allocVertices() {
    NK_ASSERT(currVertexBuffer->vertexCount + NK_CANVAS_VERTICES_PER_QUAD <=
                  NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER,
              "Error: Can't allocate more vertices.")
    NkCanvasVertex* vertices =
        &currVertexBuffer->vertices[currVertexBuffer->vertexCount];
    currVertexBuffer->vertexCount += NK_CANVAS_VERTICES_PER_QUAD;
    return vertices;
}

void NkCanvasBase::pushMatrix() { matrixStack.pushMatrix(); }

void NkCanvasBase::popMatrix() { matrixStack.popMatrix(); }

void NkCanvasBase::translate(float x, float y) { matrixStack.translate(x, y); }

void NkCanvasBase::rotate(float rad) { matrixStack.rotate(rad); }

void NkCanvasBase::scale(float x, float y) { matrixStack.scale(x, y); }

void NkCanvasBase::loadIdentity() { matrixStack.loadIdentity(); }

NkCanvasMatrix& NkCanvasBase::currentMatrix() {
    return matrixStack.currentMatrix();
}

float NkCanvasBase::width() const { return resolution[0]; }

float NkCanvasBase::height() const { return resolution[1]; }

void NkCanvasBase::beginFrame(NkCanvas* canvas) {
    if (!whiteImage) {
        const uint32_t pixels[] = {0xffffffff, 0xffffffff, 0xffffffff,
                                   0xffffffff};
        whiteImage =
            nk::canvas::createImage(canvas, NK_CANVAS_WHITE_IMAGE_WIDTH,
                                    NK_CANVAS_WHITE_IMAGE_HEIGHT, pixels);
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
        nk::canvas_internal::initTextureAtlasResource(canvas,
                                                      frameTextureAtlas);
#endif
    }
    nk::canvas_internal::waitFrameSyncPoint(
        gpuFrameSyncPoint[currentFrameIndex],
        gpuFrameWaitValue[currentFrameIndex]);
    vertexBufferAllocator[currentFrameIndex].reset();
    drawBatchArray[currentFrameIndex].reset();
    memset(&currDrawBatch, 0, sizeof(currDrawBatch));
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    frameTextureAtlas.reset();
#endif
}

void NkCanvasBase::endFrame(NkCanvas* canvas) {}

void NkCanvasBase::swapFrame(NkCanvas* canvas) {
    lastFrameIndex = currentFrameIndex;
    nk::canvas_internal::signalFrameSyncPoint(
        canvas, gpuFrameSyncPoint[currentFrameIndex], currentFrame);
    gpuFrameWaitValue[currentFrameIndex] = currentFrame;
    currentFrame++;
    currentFrameIndex = currentFrame % NK_CANVAS_MAX_FRAMES;
    currVertexBuffer = nullptr;
}

void NkCanvasBase::drawLine(float x0, float y0, float x1, float y1,
                            float lineWidth, uint32_t color) {
    float dx = x0 - x1;
    float dy = y0 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    float width = lineWidth * 0.5f;
    float lx0 = x1 - width * (y1 - y0) / len;
    float ly0 = y1 - width * (x0 - x1) / len;
    float lx1 = x0 - width * (y1 - y0) / len;
    float ly1 = y0 - width * (x0 - x1) / len;
    float lx2 = x0 + width * (y1 - y0) / len;
    float ly2 = y0 + width * (x0 - x1) / len;
    float lx3 = x1 + width * (y1 - y0) / len;
    float ly3 = y1 + width * (x0 - x1) / len;

#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    NkTextureAtlasRect textureRect = frameTextureAtlas.addImage(whiteImage);
    float u0 = (float)(textureRect.x) / frameTextureAtlas.width;
    float v0 = (float)(textureRect.y) / frameTextureAtlas.height;
    float u1 = (float)(textureRect.x + NK_CANVAS_WHITE_IMAGE_WIDTH) /
               frameTextureAtlas.width;
    float v1 = (float)(textureRect.y + NK_CANVAS_WHITE_IMAGE_HEIGHT) /
               frameTextureAtlas.height;
#else
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 1.0f;
    float v1 = 1.0f;
#endif

    NkCanvasVertex vertices[4] = {{{lx0, ly0}, {u0, v0}, color},
                                  {{lx1, ly1}, {u0, v1}, color},
                                  {{lx2, ly2}, {u1, v1}, color},
                                  {{lx3, ly3}, {u1, v0}, color}};
    pushQuad(vertices, whiteImage);
}

void NkCanvasBase::drawRect(float x, float y, float width, float height,
                            uint32_t color) {
    drawImage(x, y, width, height, 0, 0, NK_CANVAS_WHITE_IMAGE_WIDTH,
              NK_CANVAS_WHITE_IMAGE_HEIGHT, color, whiteImage);
}

void NkCanvasBase::drawImage(float x, float y, NkImage* image) {
    float width = nk::img::width(image);
    float height = nk::img::height(image);
    drawImage(x, y, width, height, 0, 0, width, height, 0xffffffff, image);
}

void NkCanvasBase::drawImage(float x, float y, uint32_t color, NkImage* image) {
    float width = nk::img::width(image);
    float height = nk::img::height(image);
    drawImage(x, y, width, height, 0, 0, width, height, color, image);
}

void NkCanvasBase::drawImage(float x, float y, float width, float height,
                             NkImage* image) {
    drawImage(x, y, width, height, 0, 0, width, height, 0xffffffff, image);
}

void NkCanvasBase::drawImage(float x, float y, float width, float height,
                             uint32_t color, NkImage* image) {
    drawImage(x, y, width, height, 0, 0, width, height, color, image);
}

void NkCanvasBase::drawImage(float x, float y, float frameX, float frameY,
                             float frameWidth, float frameHeight,
                             uint32_t color, NkImage* image) {
    float width = nk::img::width(image);
    float height = nk::img::height(image);
    drawImage(x, y, width, height, frameX, frameY, frameWidth, frameHeight,
              color, image);
}

void NkCanvasBase::drawImage(float x, float y, float width, float height,
                             float frameX, float frameY, float frameWidth,
                             float frameHeight, uint32_t color,
                             NkImage* image) {
    float imageWidth = nk::img::width(image);
    float imageHeight = nk::img::height(image);
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    NkTextureAtlasRect textureRect = frameTextureAtlas.addImage(image);
    float u0 = (textureRect.x + frameX) / frameTextureAtlas.width;
    float v0 = (textureRect.y + frameY) / frameTextureAtlas.height;
    float u1 = (textureRect.x + frameX + frameWidth) / frameTextureAtlas.width;
    float v1 =
        (textureRect.y + frameY + frameHeight) / frameTextureAtlas.height;
#else
    float u0 = frameX / imageWidth;
    float v0 = frameY / imageHeight;
    float u1 = (frameX + frameWidth) / imageWidth;
    float v1 = (frameY + frameHeight) / imageHeight;
#endif
    NkCanvasVertex vertices[4] = {{{x, y}, {u0, v0}, color},
                                  {{x, y + height}, {u0, v1}, color},
                                  {{x + width, y + height}, {u1, v1}, color},
                                  {{x + width, y}, {u1, v0}, color}};
    pushQuad(vertices, image);
}
