#pragma once

#include <nk/app.h>

struct NkCanvas;
struct NkImage;

enum class NkImageFormat {
    R8G8B8A8_UNORM
};

namespace nk {

    namespace canvas {

        void identity(NkCanvas* canvas);
        void pushMatrix(NkCanvas* canvas);
        void popMatrix(NkCanvas* canvas);
        void translate(NkCanvas* canvas, float x, float y);
        void rotate(NkCanvas* canvas, float rad);
        void scale(NkCanvas* canvas, float x, float y);
        void drawLine(NkCanvas* canvas, float x0, float y0, float x1, float y1,
                      float lineWidth, uint32_t color);
        void drawRect(NkCanvas* canvas, float x, float y, float width,
                      float height, uint32_t color);
        void drawImage(NkCanvas* canvas, float x, float y, NkImage* image);
        void drawImage(NkCanvas* canvas, float x, float y, uint32_t color,
                       NkImage* image);
        void drawImage(NkCanvas* canvas, float x, float y, float width,
                       float height, NkImage* image);
        void drawImage(NkCanvas* canvas, float x, float y, float width,
                       float height, uint32_t color, NkImage* image);
        void drawImage(NkCanvas* canvas, float x, float y, float frameX,
                       float frameY, float frameWidth, float frameHeight,
                       uint32_t color, NkImage* image);
        void drawImage(NkCanvas* canvas, float x, float y, float width,
                       float height, float frameX, float frameY,
                       float frameWidth, float frameHeight, uint32_t color,
                       NkImage* image);
        void beginFrame(NkCanvas* canvas, float r = 0.0f, float g = 0.0f,
                        float b = 0.0f, float a = 1.0f);
        void beginFrame(NkCanvas* canvas, NkImage* renderTarget, float r = 0.0f,
                        float g = 0.0f, float b = 0.0f, float a = 1.0f);
        void endFrame(NkCanvas* canvas);
        void present(NkCanvas* canvas);
        float viewWidth(NkCanvas* canvas);
        float viewHeight(NkCanvas* canvas);
        NkImage* createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                         uint32_t height);
        NkImage*
        createImage(NkCanvas* canvas, uint32_t width, uint32_t height,
                    const void* pixels,
                    NkImageFormat format = NkImageFormat::R8G8B8A8_UNORM);
        bool destroyImage(NkCanvas* canvas, NkImage* image);

    } // namespace canvas

    namespace img {

        float width(NkImage* image);
        float height(NkImage* image);

    } // namespace img

    namespace app {

        NkCanvas* canvas(NkApp* app);

    }

} // namespace nk
