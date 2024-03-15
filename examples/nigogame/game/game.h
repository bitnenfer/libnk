#pragma once

#include "actors.h"
#include "moco.h"
#include "player.h"
#include "tilemap.h"
#include <assert.h>
#include <chrono>
#include <nk/app.h>
#include <nk/canvas.h>
#include <nk/hid.h>
#include <stdint.h>
#include <unordered_map>
#include <vector>

enum class NcSceneState {
    NONE,
    MAIN_MENU,
    GAME
};

enum class NcGameState {
    PLAYING,
    LOSE,
    WIN
};

struct NcGame;
struct NcScene {
    virtual void init() {}
    virtual void destroy();
    virtual void update() {}
    virtual void render() {}

    NkImage* addImage(const std::string& name, uint32_t width, uint32_t height,
                      const void* pixels);
    NkImage* getImage(const std::string& name);

    std::unordered_map<std::string, NkImage*> loadedImages;
    NcGame* game;
};

struct NcMainMenuScene : public NcScene {
    void init() override;
    void destroy() override;
    void update() override;
    void render() override;
    NkImage* nigoIcon;
    float fadeScreen;
    bool startFading;
};

struct NcGameScene : public NcScene {
    void init() override;
    void destroy() override;
    void update() override;
    void render() override;

    NcParticleEmitter mocoParticle;
    NcObjectAllocator<NcMoco> mocoAllocator;
    NcMoco* activeMocos[512];
    uint32_t activeMocoNum;
    NcPlayer player;
    NcTilemap tilemap;
    NcGameState gameState;
    NkImage* bombIcon;
    NkImage* nigoIcon;
    float fadeScreen;
};

struct NcGame {
    void init();
    void destroy();
    void update();
    void run();
    void switchScene(NcSceneState scene);
    double getSeconds() const;
    float getElapsedTime() const;
    float randomFloat() const;
    float getGravity() const { return 0.2f; }

public:
    NcScene* currentScene;
    NkApp* app;
    NkHID* hid;
    NkCanvas* canvas;
    NkImage* renderTarget;
    NcVector mouse;
    float width;
    float height;

private:
    NcMainMenuScene mainMenuScene;
    NcGameScene gameScene;
    NcSceneState currentState;
    NcSceneState nextState;
    float gameScale;
    uint32_t frameCount;
    double targetFrameTime;
    double elapsedTime;
    double accumulatedTime;
    double startTime;
    double endTime;
};