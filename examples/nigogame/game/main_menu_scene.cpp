#include "assets.h"
#include "game.h"

void NcMainMenuScene::init() {
    nk::hid::showCursor(game->hid, true);
    nigoIcon = addImage("nigo_icon", image_nigo_head_width,
                        image_nigo_head_height, image_nigo_head);
    fadeScreen = 0.0f;
    startFading = false;
}
void NcMainMenuScene::destroy() {}
void NcMainMenuScene::update() {
    NcScene::update();
    NkHID* hid = game->hid;
    if (nk::hid::mouseClick(hid, NkMouseButton::LEFT)) {
        startFading = true;
    }
#if _WIN32
    if (nk::hid::keyClick(hid, NkKeyCode::ESC)) {
        nk::app::quit(game->app);
    }
#endif
    if (startFading) {
        if (fadeScreen >= 1.0f) {
            game->switchScene(NcSceneState::GAME);
        }
        fadeScreen += game->getElapsedTime();
        if (fadeScreen > 1.0f) {
            fadeScreen = 1.0f;
        }
    }
}
void NcMainMenuScene::render() {
    NcScene::render();
    static float rotation = 0.0f;
    NkCanvas* canvas = game->canvas;

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, cosf(rotation * 2) * 10,
                          sinf(rotation * 2) * 10);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, game->width / 2, game->height / 2);
    nk::canvas::rotate(canvas, -rotation * 0.1f);
    nk::canvas::drawRect(canvas, -80, -80, 160, 160, NK_COLOR_UINT(0x3c291eff));
    nk::canvas::popMatrix(canvas);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, game->width / 2, game->height / 2);
    nk::canvas::rotate(canvas, rotation);
    nk::canvas::drawRect(canvas, -60, -60, 120, 120, NK_COLOR_UINT(0x46362cff));
    nk::canvas::popMatrix(canvas);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, game->width / 2, game->height / 2);
    nk::canvas::rotate(canvas, -rotation * 0.25f);
    nk::canvas::drawRect(canvas, -50, -50, 100, 100, NK_COLOR_UINT(0xa16920ff));
    nk::canvas::popMatrix(canvas);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, game->width / 2, game->height / 2);
    nk::canvas::scale(canvas, 4.0f, 4.0f);
    nk::canvas::drawImage(canvas, -nk::img::width(nigoIcon) * 0.5f,
                          -nk::img::height(nigoIcon) * 0.5f, nigoIcon);
    nk::canvas::popMatrix(canvas);
    nk::canvas::popMatrix(canvas);

    if (startFading) {
        nk::canvas::drawRect(canvas, 0, 0, game->width, game->height,
                             NK_COLOR_RGBA_FLOAT(0, 0, 0, fadeScreen));
    }

    rotation += game->getElapsedTime() * 0.5f;
}