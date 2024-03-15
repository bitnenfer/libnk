#pragma once

#include "actors.h"

struct NkImage;
struct NcGame;

struct NcMoco {

    static constexpr uint32_t COLOR = NK_COLOR_UINT(0x92c111ff);

    void init(NcGame* game, float x, float y);
    void destroy();
    void update();
    void render();
    void testTarget(NcActor* actor);
    void kill();

    NcGame* game;
    NcActor* target;
    NkImage* body;
    NkImage* bodyAngry;
    NkImage* leftHand;
    NkImage* rightHand;
    NcActor bodyActor;
    NcVector drop;
    float dropVelocity;
    float dropTime;
    float time;
    float direction;
    float distanceToTarget;
    float movementOffset;
    float explosionTimer;
    bool startExplodingTimer;
    bool exploding;
    bool alive;
};