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
    float scale;
    uint32_t color;
    uint32_t imageIndex;
};

#define SPRITE_COUNT 10000
static Sprite sprites[SPRITE_COUNT];

int main() {
    NkApp* app = nk::app::create({960, 640, "Demo", false, 0, true});
    NkCanvas* canvas = nk::app::canvas(app);
    NkHID* hid = nk::app::hid(app);

    NkImage* images[4] = {};

    images[0] = nk::canvas::createImage(canvas, image_img1_width,
                                        image_img1_height, image_img1);
    images[1] = nk::canvas::createImage(canvas, image_img2_width,
                                        image_img2_height, image_img2);
    images[2] = nk::canvas::createImage(canvas, image_img3_width,
                                        image_img3_height, image_img3);
    images[3] = nk::canvas::createImage(canvas, image_img4_width,
                                        image_img4_height, image_img4);

    float rotation = 0.0f;

    srand((unsigned int)time(NULL));

    for (uint32_t index = 0; index < SPRITE_COUNT; ++index) {
        sprites[index].x =
            ((float)rand() / (float)RAND_MAX) * nk::canvas::viewWidth(canvas);
        sprites[index].y =
            ((float)rand() / (float)RAND_MAX) * nk::canvas::viewHeight(canvas);
        sprites[index].rotation = ((float)rand() / (float)RAND_MAX) * 3.14f;
        sprites[index].color = NK_COLOR_RGB_UINT(
            rand() % RAND_MAX, rand() % RAND_MAX, rand() % RAND_MAX);
        sprites[index].scale = (float)rand() / (float)RAND_MAX + 0.2f;
        sprites[index].imageIndex = rand() % 4;
    }

#if __EMSCRIPTEN__
    struct AppData {
        NkApp* app;
        NkCanvas* canvas;
        NkHID* hid;
        NkImage** images;
        float& rotation;
    };
    AppData appData = {app, canvas, hid, &images[0], rotation};
    emscripten_set_main_loop_arg(
        [](void* userData) {
            AppData* appData = (AppData*)userData;
            NkApp* app = appData->app;
            NkCanvas* canvas = appData->canvas;
            NkHID* hid = appData->hid;
            NkImage** images = appData->images;
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
                nk::canvas::scale(canvas, sprite.scale, sprite.scale);
                nk::canvas::drawImage(
                    canvas, -nk::img::width(images[sprite.imageIndex]) * 0.5f,
                    -nk::img::height(images[sprite.imageIndex]) * 0.5f,
                    sprite.color, images[sprite.imageIndex]);
                nk::canvas::popMatrix(canvas);
                sprite.rotation -= 1.0f / 60.0f;
            }

            nk::canvas::pushMatrix(canvas);
            nk::canvas::translate(canvas, nk::hid::mouseX(hid),
                                  nk::hid::mouseY(hid));
            nk::canvas::rotate(canvas, rotation);
            nk::canvas::drawImage(canvas, -nk::img::width(images[0]) * 0.5f,
                                  -nk::img::height(images[0]) * 0.5f,
                                  images[0]);
            nk::canvas::popMatrix(canvas);
            nk::canvas::endFrame(canvas);
            nk::canvas::present(canvas);

            rotation += 1.0f / 60.0f;

#if __EMSCRIPTEN__
        },
        &appData, 60, 1);
#else
    }
#endif

    for (uint32_t index = 0; index < 4; ++index) {
        nk::canvas::destroyImage(canvas, images[index]);
    }
    nk::app::destroy(app);
    return 0;
}
