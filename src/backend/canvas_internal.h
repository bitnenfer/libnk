#pragma once

#include "texture_packer.h"
#include "utils.h"
#include <math.h>
#include <nk/canvas.h>

#define NK_CANVAS_INDICES_PER_QUAD  6
#define NK_CANVAS_VERTICES_PER_QUAD 4
#define NK_CANVAS_MAX_DRAW_ELEMENTS_PER_BUFFER                                 \
    (1 << 16) // Seems to be a good balance
#define NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER                                    \
    (NK_CANVAS_MAX_DRAW_ELEMENTS_PER_BUFFER * NK_CANVAS_VERTICES_PER_QUAD)
#define NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER_BYTE_SIZE                          \
    (NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER * sizeof(NkCanvasVertex))
#define NK_CANVAS_MAX_VERTEX_BUFFERS (1 << 7)
#define NK_CANVAS_MAX_INDICES                                                  \
    (NK_CANVAS_MAX_DRAW_ELEMENTS_PER_BUFFER * NK_CANVAS_INDICES_PER_QUAD)
#define NK_CANVAS_INDEX_TYPE uint32_t
#define NK_CANVAS_INDEX_SIZE (sizeof(NK_CANVAS_INDEX_TYPE))
#define NK_CANVAS_MAX_INDICES_BYTE_SIZE                                        \
    (NK_CANVAS_MAX_INDICES * NK_CANVAS_INDEX_SIZE)
#define NK_CANVAS_MAX_BATCHES            (1 << 12)
#define NK_CANVAS_MAX_MATRIX_STACK_DEPTH (1 << 10)
#define NK_CANVAS_MAX_FRAMES             2
#define NK_CANVAS_WHITE_IMAGE_WIDTH      2
#define NK_CANVAS_WHITE_IMAGE_HEIGHT     2

#ifdef NK_CANVAS_BINDLESS_RESOURCE_ENABLED
#undef NK_CANVAS_BINDLESS_RESOURCE_ENABLED
#define NK_CANVAS_BINDLESS_RESOURCE_ENABLED 1
#endif

typedef void* NkGPUHandle;

struct NkCanvasDrawBatchInternal {
    NkImage* image;
    struct NkCanvasVertexBuffer* buffer;
    uint32_t bufferOffset;
    uint32_t count;
};

struct NkCanvasVertex {
    float position[2];
    float texCoord[2];
    uint32_t color;
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
    uint32_t textureId;
#endif
};

struct NkCanvasVertexBuffer {
    void* gpuVertexBuffer;
    uint32_t vertexCount;
    NkCanvasVertex* vertices;
};

struct NkCanvasDrawBatchInternalArray {

    void init();
    void destroy();
    void reset();
    void add(const NkCanvasDrawBatchInternal& drawBatch);
    NkCanvasDrawBatchInternal* last();

    NkCanvasDrawBatchInternal* drawBatches;
    uint32_t drawBatchNum;
};

struct NkCanvasVertexBufferAllocator {

    void init();
    void destroy();
    void reset();
    NkCanvasVertexBuffer* allocate();

    uint32_t vertexBufferNum;

private:
    NkCanvasVertexBuffer* vertexBuffers;
    uint32_t vertexBufferMax;
};

struct NkCanvasMatrix {

    inline void init() {
        float id[] = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
        load(id);
    }
    inline void load(float matrix[6]) {
        components[0] = matrix[0];
        components[1] = matrix[1];
        components[2] = matrix[2];
        components[3] = matrix[3];
        components[4] = matrix[4];
        components[5] = matrix[5];
    }
    inline void identity() {
        float id[] = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
        load(id);
    }
    inline void translate(float x, float y) {
        float ttx = a * x + c * y + tx;
        float tty = b * x + d * y + ty;
        tx = ttx;
        ty = tty;
    }
    inline void rotate(float rad) {
        float cr = cosf(rad);
        float sr = sinf(rad);
        float m0 = cr * a + sr * c;
        float m1 = cr * b + sr * d;
        float m2 = -sr * a + cr * c;
        float m3 = -sr * b + cr * d;
        a = m0;
        b = m1;
        c = m2;
        d = m3;
    }
    inline void scale(float x, float y) {
        a = a * x;
        b = b * x;
        c = c * y;
        d = d * y;
    }
    inline float operator[](uint32_t index) { return components[index]; }

    inline void multiply(float& x, float& y) {
        float inX = x;
        float inY = y;
        float tempX = inX * a + inY * c + tx;
        float tempY = inX * b + inY * d + ty;
        x = tempX;
        y = tempY;
    }

    union {
        float components[6];
        struct {
            float a, b, c, d, tx, ty;
        };
    };
};

struct NkCanvasMatrixStack {

    inline void init() { current.init(); }
    inline void pushMatrix() {
        if (depth < NK_CANVAS_MAX_MATRIX_STACK_DEPTH) {
            matrices[depth++] = current;
        }
    }
    inline void popMatrix() {
        if (depth > 0) {
            current = matrices[--depth];
        }
    }
    inline void translate(float x, float y) { current.translate(x, y); }
    inline void rotate(float rad) { current.rotate(rad); }
    inline void scale(float x, float y) { current.scale(x, y); }
    inline void loadIdentity() { current.identity(); }
    inline NkCanvasMatrix& currentMatrix() { return current; }

    NkCanvasMatrix matrices[NK_CANVAS_MAX_MATRIX_STACK_DEPTH];
    NkCanvasMatrix current;
    uint32_t depth;
};

struct NkCanvasBase {

    void init(NkCanvas* canvas, float width, float height);
    void destroy(NkCanvas* canvas);
    void waitCurrentFrame();
    void signalCurrentFrame();
    NkCanvasVertexBuffer* allocateVertexBuffer();
    const void* indexBufferData() const;
    void pushQuad(const NkCanvasVertex* vertices, NkImage* image);
    NkCanvasDrawBatchInternal*
    addDrawBatch(const NkCanvasDrawBatchInternal& drawBatch);
    NkCanvasDrawBatchInternal* nextDrawBatch(NkImage* image);
    NkCanvasVertex* allocVertices();
    float width() const;
    float height() const;
    void pushMatrix();
    void popMatrix();
    void translate(float x, float y);
    void rotate(float rad);
    void scale(float x, float y);
    void loadIdentity();
    NkCanvasMatrix& currentMatrix();
    void beginFrame(NkCanvas* canvas);
    void endFrame(NkCanvas* canvas);
    void swapFrame(NkCanvas* canvas);
    void drawLine(float x0, float y0, float x1, float y1, float lineWidth,
                  uint32_t color);
    void drawRect(float x, float y, float width, float height, uint32_t color);
    void drawImage(float x, float y, NkImage* image);
    void drawImage(float x, float y, uint32_t color, NkImage* image);
    void drawImage(float x, float y, float width, float height, NkImage* image);
    void drawImage(float x, float y, float width, float height, uint32_t color,
                   NkImage* image);
    void drawImage(float x, float y, float frameX, float frameY,
                   float frameWidth, float frameHeight, uint32_t color,
                   NkImage* image);
    void drawImage(float x, float y, float width, float height, float frameX,
                   float frameY, float frameWidth, float frameHeight,
                   uint32_t color, NkImage* image);

public:
    NkCanvas* canvas;
    NkImage* whiteImage;
    NkGPUHandle gpuIndexBuffer;
    uint64_t currentFrameIndex;
    uint64_t lastFrameIndex;
    bool indicesUploaded;
    NkCanvasVertexBufferAllocator vertexBufferAllocator[NK_CANVAS_MAX_FRAMES];
    NkCanvasDrawBatchInternalArray drawBatchArray[NK_CANVAS_MAX_FRAMES];
    NkGPUHandle gpuFrameSyncPoint[NK_CANVAS_MAX_FRAMES];
    uint64_t gpuFrameWaitValue[NK_CANVAS_MAX_FRAMES];
    NkCanvasVertexBuffer* currVertexBuffer;
    NkCanvasDrawBatchInternal* currDrawBatch;
    uint64_t currentFrame;
    float resolution[2];
    NkCanvasMatrixStack matrixStack;
    NK_CANVAS_INDEX_TYPE* indices;
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
    NkTextureAtlas frameTextureAtlas;
#endif
};

namespace nk {

    namespace canvas_internal {

        void initVertexBuffer(
            NkCanvasVertexBuffer* vertexBuffer,
            size_t bufferSize = NK_CANVAS_MAX_DRAW_VERTS_PER_BUFFER_BYTE_SIZE);
        void initIndexBuffer(void** gpuIndexBuffer);
        void destroyVertexBuffer(NkCanvasVertexBuffer* vertexBuffer);
        void destroyIndexBuffer(void** gpuIndexBuffer);
        void signalFrameSyncPoint(NkCanvas* canvas, NkGPUHandle gpuSyncPoint,
                                  uint64_t value);
        void waitFrameSyncPoint(NkGPUHandle gpuSyncPoint, uint64_t value);
        void initFrameSyncPoint(NkGPUHandle* gpuSyncPoint);
        void destroyFrameSyncPoint(NkGPUHandle* gpuSyncPoint);
        NkCanvasBase* canvasBase(NkCanvas* canvas);
#if NK_CANVAS_TEXTURE_ATLAS_ENABLED
        void initTextureAtlasResource(NkCanvas* canvas,
                                      NkTextureAtlas& textureAtlas);
        void destroyTextureAtlasResource(NkTextureAtlas& textureAtlas);
        void setTextureAtlasState(NkImage* image,
                                  const NkTextureAtlasRect& rect);
        bool isImageInTextureAtlas(NkImage* image);
        const NkTextureAtlasRect& textureRect(NkImage* image);
#endif
#if NK_CANVAS_BINDLESS_RESOURCE_ENABLED
        uint32_t addImageToTable(NkCanvas* canvas, NkImage* image);
#endif

    } // namespace canvas_internal

} // namespace nk
