#include "game.h"
#include <chrono>
#include <random>
#include <stdlib.h>
#include <thread>

#if __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

void NcScene::destroy() {
    for (auto& entry : loadedImages) {
        nk::canvas::destroyImage(game->canvas, entry.second);
    }
    loadedImages.clear();
}

NkImage* NcScene::addImage(const std::string& name, uint32_t width,
                           uint32_t height, const void* pixels) {
    const auto& entry = loadedImages.find(name);
    if (entry == loadedImages.end()) {
        NkImage* image =
            nk::canvas::createImage(game->canvas, width, height, pixels);
        loadedImages.insert({name, image});
        return image;
    }
    return entry->second;
}

NkImage* NcScene::getImage(const std::string& name) {
    const auto& entry = loadedImages.find(name);
    if (entry != loadedImages.end()) {
        return entry->second;
    }
    return nullptr;
}

void NcGame::init() {
    app = nk::app::create({600, 900, "NIGO", false, 0, false, false});
    canvas = nk::app::canvas(app);
    hid = nk::app::hid(app);
    currentState = NcSceneState::NONE;
    nextState = NcSceneState::MAIN_MENU;
    currentScene = nullptr;
    gameScale = 2.0f;
    renderTarget = nk::canvas::createRenderTargetImage(
        canvas, 640 / (uint32_t)gameScale, 960 / (uint32_t)gameScale);
    accumulatedTime = 0.0f;
    targetFrameTime = 1.0f / 60.0f;
    startTime = getSeconds();
    endTime = getSeconds();
    width = nk::img::width(renderTarget);
    height = nk::img::height(renderTarget);
    nk::hid::showCursor(hid, false);
}

void NcGame::destroy() {
    nk::canvas::destroyImage(canvas, renderTarget);
    nk::app::destroy(app);
}

void NcGame::update() {
    endTime = getSeconds();
    elapsedTime = endTime - startTime;
    startTime = getSeconds();

    accumulatedTime += elapsedTime;

    if (frameCount == 0 || accumulatedTime >= targetFrameTime) {
        nk::app::update(app);
        mouse.x = nk::hid::mouseX(hid) / gameScale;
        mouse.y = nk::hid::mouseY(hid) / gameScale;

        accumulatedTime -= targetFrameTime;
        elapsedTime = targetFrameTime;

        if (nextState != currentState) {
            if (currentScene) {
                currentScene->destroy();
                currentScene->game = nullptr;
            }
            switch (nextState) {
            case NcSceneState::GAME:
                currentScene = &gameScene;
                break;
            case NcSceneState::MAIN_MENU:
                currentScene = &mainMenuScene;
                break;
            case NcSceneState::NONE:
                currentScene = nullptr;
                break;
            }
            currentState = nextState;
            if (currentScene) {
                currentScene->game = this;
                currentScene->init();
            }
        }

        if (currentScene) {
            currentScene->update();
        }

        // Render Game
        nk::canvas::beginFrame(canvas, renderTarget);
        nk::canvas::drawRect(canvas, 0.0f, 0.0f, width, height,
                             NK_COLOR_UINT(0x241f1cff));
        {
            nk::canvas::pushMatrix(canvas);
            if (currentScene) {
                currentScene->render();
            }
            nk::canvas::popMatrix(canvas);
        }
        nk::canvas::endFrame(canvas);
    }

    // Render Game Texture
    nk::canvas::beginFrame(canvas);
    {
        nk::canvas::pushMatrix(canvas);
        nk::canvas::scale(canvas, gameScale, gameScale);
        nk::canvas::drawImage(canvas, 0, 0, renderTarget);
        nk::canvas::popMatrix(canvas);
    }
    nk::canvas::endFrame(canvas);

    nk::canvas::present(canvas);
    frameCount++;

    if (nk::app::shouldQuit(app)) {
        if (currentScene) {
            currentScene->destroy();
        }
    }
}

void NcGame::run() {
#if __EMSCRIPTEN__
    emscripten_set_main_loop_arg(
        [](void* userData) { ((NcGame*)userData)->update(); }, this, 60, 1);
#else
    while (!nk::app::shouldQuit(app)) {
        update();
    }
#endif
}

void NcGame::switchScene(NcSceneState scene) { nextState = scene; }
double NcGame::getSeconds() const {
    double time = std::chrono::time_point_cast<std::chrono::duration<double>>(
                      std::chrono::high_resolution_clock::now())
                      .time_since_epoch()
                      .count();
    return time;
}
float NcGame::getElapsedTime() const { return (float)elapsedTime; }

float NcGame::randomFloat() const {
    static std::random_device randDevice;
    static std::mt19937 mtGen(randDevice());
    static std::uniform_real_distribution<float> udt;
    return udt(mtGen);
}
