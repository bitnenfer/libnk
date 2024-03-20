#include <nk/app.h>
#include <nk/canvas.h>
#include <nk/hid.h>

#include <stdlib.h>
#include <time.h>

#include "image.h"

#if __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

struct Sprite {
    float x;
    float y;
    float rotation;
    uint32_t color;
};

#define SPRITE_COUNT 1000
static Sprite sprites[SPRITE_COUNT];

int main() {
    NkApp* app = nk::app::create({960, 640, "Demo"});
    NkCanvas* canvas = nk::app::canvas(app);
    NkHID* hid = nk::app::hid(app);
    NkImage* image = nk::canvas::createImage(canvas, image_img_width,
                                             image_img_height, image_img);

    float rotation = 0.0f;

    srand((uint32_t)time(NULL));

    for (uint32_t index = 0; index < SPRITE_COUNT; ++index) {
        sprites[index].x =
            ((float)rand() / (float)RAND_MAX) * nk::canvas::viewWidth(canvas);
        sprites[index].y =
            ((float)rand() / (float)RAND_MAX) * nk::canvas::viewHeight(canvas);
        sprites[index].rotation = ((float)rand() / (float)RAND_MAX) * 3.14f;
        sprites[index].color = NK_COLOR_RGB_UINT(
            rand() % RAND_MAX, rand() % RAND_MAX, rand() % RAND_MAX);
    }

#if __EMSCRIPTEN__
    struct AppData {
        NkApp* app;
        NkCanvas* canvas;
        NkHID* hid;
        NkImage* image;
        float& rotation;
    };
    AppData appData = {app, canvas, hid, image, rotation};
    emscripten_set_main_loop_arg(
        [](void* userData) {
            AppData* appData = (AppData*)userData;
            NkApp* app = appData->app;
            NkCanvas* canvas = appData->canvas;
            NkHID* hid = appData->hid;
            NkImage* image = appData->image;
            float& rotation = appData->rotation;
#else
    while (!nk::app::shouldQuit(app)) {
#endif
            nk::app::update(app);

            if (nk::hid::keyClick(hid, NkKeyCode::ESC)) {
                nk::app::quit(app);
            }

            nk::canvas::beginFrame(canvas);

            for (uint32_t index = 0; index < SPRITE_COUNT; ++index) {
                Sprite& sprite = sprites[index];
                nk::canvas::pushMatrix(canvas);
                nk::canvas::translate(canvas, sprite.x, sprite.y);
                nk::canvas::rotate(canvas, sprite.rotation);
                nk::canvas::drawImage(canvas, -nk::img::width(image) * 0.5f,
                                      -nk::img::height(image) * 0.5f,
                                      sprite.color, image);
                nk::canvas::popMatrix(canvas);
                sprite.rotation -= 1.0f / 60.0f;
            }

            nk::canvas::pushMatrix(canvas);
            nk::canvas::translate(canvas, nk::hid::mouseX(hid),
                                  nk::hid::mouseY(hid));
            nk::canvas::rotate(canvas, rotation);

            nk::canvas::drawImage(canvas, -nk::img::width(image) * 0.5f,
                                  -nk::img::height(image) * 0.5f, image);

            nk::canvas::popMatrix(canvas);
            nk::canvas::endFrame(canvas);
            nk::canvas::present(canvas);

            rotation += 1.0f / 60.0f;

#if __EMSCRIPTEN__
        },
        &appData, 0, 1);
#else
    }
#endif

    nk::canvas::destroyImage(canvas, image);
    nk::app::destroy(app);
    return 0;
}
