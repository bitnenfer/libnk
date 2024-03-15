#pragma once

#include "actors.h"
#include <array>

struct NcScene;
struct NcPlayer;
struct NcTilemap;
struct NcTile;

struct NcExplosionPoint {
    float x;
    float y;
    uint32_t color;
};

struct NcParticle : public NcActor {

    virtual void render(NcGame* game);
    float lifetime;
    uint32_t color;
};

struct NcParticleEmitter {

    void init(NcGame* game);
    void destroy();
    void render(float viewX, float viewY, float viewWidth, float viewHeight);
    void update();
    void explode(float x, float y, uint32_t color, uint32_t count = 10);

    NcPool<NcParticle> pool;
    std::vector<NcParticle*> activeParticles;
    std::array<NcParticle, 128> particles;
    NcGame* game;
};

struct NcBomb {
    void init(NcPlayer* player, NcGame* game, NkImage* image);
    void reset(float x, float y);
    void update(NcGame* game);
    void render(NcGame* game);

    NcPlayer* player;
    NcSprite sprite;
    float life;
};

struct NcPlayer {
    void init(NcScene* scene, float x, float y);
    void destroy();
    void update(NcTilemap* tilemap);
    void render(float viewX, float viewY, float viewWidth, float viewHeight);
    void spawnBomb(float x, float y, float angle, float distanceToMouse);
    void recycle(NcBomb* bomb);
    void kill();

    NcParticleEmitter bombParticles;
    NcParticleEmitter digParticles;
    std::vector<NcBomb*> bombs;
    std::vector<NcBomb*> recycledBombs;
    std::array<NcBomb, 32> bombPool;
    std::vector<NcExplosionPoint> explosionPoints;
    NcSprite sprite;
    NkImage* explosionImage;
    NcGame* game;
    NcTile* activeTile;
    bool isThrowing;
    bool isDigging;
    bool wasBombThrown;
    bool isReadyToShoot;
    bool alive;
    float shakeTime;
    float fadeTime;
    float digParticleDelay;
    float bombTimer;
    float life;
};
