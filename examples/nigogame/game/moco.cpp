#include "moco.h"
#include "assets.h"
#include "game.h"
#include <float.h>
// 0x92c111ff = color
void NcMoco::init(NcGame* game, float x, float y) {
    this->game = game;
    body =
        game->currentScene->addImage("moco_body", image_moco_body_width,
                                     image_moco_body_height, image_moco_body);
    bodyAngry = game->currentScene->addImage(
        "moco_body_angry", image_moco_body_angry_width,
        image_moco_body_angry_height, image_moco_body_angry);
    leftHand =
        game->currentScene->addImage("moco_lhand", image_moco_lhand_width,
                                     image_moco_lhand_height, image_moco_lhand);
    rightHand =
        game->currentScene->addImage("moco_rhand", image_moco_rhand_width,
                                     image_moco_rhand_height, image_moco_rhand);
    bodyActor.init(game, x, y, nk::img::width(body), nk::img::height(body));
    // bodyActor.debugDraw = true;
    time = 0.0f;
    dropTime = 0.0f;
    dropVelocity = 0.0f;
    drop = {4.0f + game->randomFloat() * 48.0f, 54.0f};
    target = nullptr;
    direction = 1.0f;
    distanceToTarget = FLT_MAX;
    exploding = false;
    movementOffset = game->randomFloat() * 10.0f;
    alive = true;
    explosionTimer = 0.0f;
    startExplodingTimer = false;
}
void NcMoco::destroy() {}
void NcMoco::update() {
    if (exploding || !alive)
        return;

    time += game->getElapsedTime() * 5.0f;
    dropTime += game->getElapsedTime();
    if (dropTime > 0.25f) {
        dropTime = 0.0f;
        drop = {4.0f + game->randomFloat() * 48.0f, 54.0f};
        dropVelocity = 0.0f;
    }
    dropVelocity += game->getGravity();
    drop.y += dropVelocity;

    if (target) {
        float dx = (bodyActor.position.x -
                    (target->position.x - target->width * 0.5f));
        float dy = (bodyActor.position.y -
                    (target->position.y - target->height * 0.5f));
        float angle = atan2(dy, dx);
        bodyActor.velocity = {cosf(angle) * -1.0f, sinf(angle) * -1.0f};
        if (dx < 0.0f) {
            direction = -1.0f;
        } else {
            direction = 1.0f;
        }
        distanceToTarget = sqrtf(dx * dx + dy * dy);
        if (distanceToTarget < 60.0f) {
            startExplodingTimer = true;
        } else if (distanceToTarget > 400.0f) {
            target = nullptr;
        }
    } else {
        bodyActor.velocity = {sinf(time * 0.5f + movementOffset) * 1.0f, 0};
        if (bodyActor.velocity.x < 0.0f) {
            direction = 1.0f;
        } else {
            direction = -1.0f;
        }
        distanceToTarget = FLT_MAX;
    }

    if (startExplodingTimer) {
        explosionTimer += game->getElapsedTime();
        if (explosionTimer > 2.0f) {
            exploding = true;
        }
    }

    bodyActor.update(game);
}
void NcMoco::render() {
    if (exploding || !alive)
        return;
    NkCanvas* canvas = game->canvas;
    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, bodyActor.position.x, bodyActor.position.y);

    if (target) {
        float shakeScale =
            (1.0f - (std::min(distanceToTarget, 400.0f) / 400.0f)) * 4.0f;
        if (explosionTimer > 0.0f) {
            shakeScale *= 2.0f;
        }
        nk::canvas::translate(canvas, shakeScale * game->randomFloat(),
                              shakeScale * game->randomFloat());
    }

    nk::canvas::translate(canvas, (nk::img::width(body) * 0.5f), 0.0f);

    float dropScale = 1.0f - (dropTime / 0.25f);
    nk::canvas::drawRect(canvas, drop.x + (nk::img::width(body) * -0.5f),
                         drop.y, 4.0f * dropScale, 4.0f * dropScale,
                         NcMoco::COLOR);

    nk::canvas::scale(canvas, direction, 1.0f);

    float colorOffset = 0.5f + (movementOffset / 10.0f) * 0.5f;
    float color[3] = {colorOffset, colorOffset, colorOffset};
    NkImage* bodyToRender = body;
    if (explosionTimer > 0.0f) {
        bodyToRender = bodyAngry;
    }
    nk::canvas::drawImage(
        canvas, cosf(time + 3.0f) + (nk::img::width(body) * -0.5f),
        sinf(time * 3.0f), NK_COLOR_RGB_FLOAT(color[0], color[1], color[2]),
        bodyToRender);

    float hx = cosf(-time) * 3.0f;
    float hy = sinf(-time) * 3.0f;

    nk::canvas::drawImage(
        canvas, 30 + hx + (nk::img::width(body) * -0.5f), 45 + hy,
        NK_COLOR_RGB_FLOAT(color[0], color[1], color[2]), leftHand);

    hx = cosf(-time + 10.0f) * 3.0f;
    hy = sinf(-time + 10.0f) * 3.0f;
    nk::canvas::drawImage(
        canvas, -10 + hx + (nk::img::width(body) * -0.5f), 45 + hy,
        NK_COLOR_RGB_FLOAT(color[0], color[1], color[2]), rightHand);
    nk::canvas::popMatrix(canvas);
    bodyActor.render(game);
}

void NcMoco::testTarget(NcActor* actor) {
    if (!actor || target)
        return;
    float dx = (bodyActor.position.x - actor->position.x);
    float dy = (bodyActor.position.y - actor->position.y);
    distanceToTarget = sqrtf(dx * dx + dy * dy);
    if (distanceToTarget < 200.0f) {
        target = actor;
    }
}

void NcMoco::kill() { alive = false; }