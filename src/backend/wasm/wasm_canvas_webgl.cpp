#include "wasm_canvas_webgl.h"

NkCanvas* nk::canvas::create(NkApp* app, bool allowResize) { return nullptr; }
bool nk::canvas::destroy(NkCanvas* canvas) { return false; }
void nk::canvas::identity(NkCanvas* canvas) {}
void nk::canvas::pushMatrix(NkCanvas* canvas) {}
void nk::canvas::popMatrix(NkCanvas* canvas) {}
void nk::canvas::translate(NkCanvas* canvas, float x, float y) {}
void nk::canvas::rotate(NkCanvas* canvas, float rad) {}
void nk::canvas::scale(NkCanvas* canvas, float x, float y) {}
void nk::canvas::drawLine(NkCanvas* canvas, float x0, float y0, float x1,
                          float y1, float lineWidth, uint32_t color) {}
void nk::canvas::drawRect(NkCanvas* canvas, float x, float y, float width,
                          float height, uint32_t color) {}
void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, NkImage* image) {
}
void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, uint32_t color,
                           NkImage* image) {}
void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, NkImage* image) {}
void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, uint32_t color, NkImage* image) {}
void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float frameX,
                           float frameY, float frameWidth, float frameHeight,
                           uint32_t color, NkImage* image) {}
void nk::canvas::drawImage(NkCanvas* canvas, float x, float y, float width,
                           float height, float frameX, float frameY,
                           float frameWidth, float frameHeight, uint32_t color,
                           NkImage* image) {}
void nk::canvas::beginFrame(NkCanvas* canvas, float r, float g, float b,
                            float a) {}
void nk::canvas::beginFrame(NkCanvas* canvas, NkImage* renderTarget, float r,
                            float g, float b, float a) {}
void nk::canvas::endFrame(NkCanvas* canvas) {}
void nk::canvas::present(NkCanvas* canvas) {}
float nk::canvas::viewWidth(NkCanvas* canvas) { return 0.0f; }
float nk::canvas::viewHeight(NkCanvas* canvas) { return 0.0f; }
NkImage* nk::canvas::createRenderTargetImage(NkCanvas* canvas, uint32_t width,
                                             uint32_t height) {
    return nullptr;
}
NkImage* nk::canvas::createImage(NkCanvas* canvas, uint32_t width,
                                 uint32_t height, const void* pixels,
                                 NkImageFormat format) {
    return nullptr;
}
bool nk::canvas::destroyImage(NkCanvas* canvas, NkImage* image) { return true; }

float nk::img::width(NkImage* image) { return 0.0f; }
float nk::img::height(NkImage* image) { return 0.0f; }
