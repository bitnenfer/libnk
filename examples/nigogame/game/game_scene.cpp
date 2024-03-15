#include "assets.h"
#include "game.h"

void NcGameScene::init() {
    NcScene::init();

    player.init(this, game->width / 2.0f, game->height / 2.0f);
    tilemap.init(game);
    tilemap.generate(45, 128);
    fadeScreen = 0.0f;
    gameState = NcGameState::PLAYING;
    mocoAllocator.init(512);
    activeMocoNum = 0;

    for (uint32_t index = 0; index < 20; ++index) {
        activeMocos[index] = mocoAllocator.create();
        activeMocos[index]->init(
            game, game->randomFloat() * tilemap.width,
            tilemap.height * 0.20f +
                (game->randomFloat() * (tilemap.height * 0.80f)));
        activeMocoNum++;
    }
    mocoParticle.init(game);
    bombIcon = getImage("bomb");
    nigoIcon = addImage("nigo_head", image_nigo_head_width,
                        image_nigo_head_height, image_nigo_head);
    nk::hid::showCursor(game->hid, false);
}
void NcGameScene::destroy() {
    mocoParticle.destroy();
    player.destroy();
    tilemap.destroy();
    mocoAllocator.destroy();
    NcScene::destroy();
}
void NcGameScene::update() {
    NcScene::update();
    NkHID* hid = game->hid;

    if (gameState == NcGameState::PLAYING) {
        if (nk::hid::keyClick(hid, NkKeyCode::ESC)) {
            game->switchScene(NcSceneState::MAIN_MENU);
        }

        player.update(&tilemap);
        if (player.sprite.debugDraw) {
            return;
        }
        player.life -= game->getElapsedTime() * 0.005f;
        if (player.life <= 0.0f) {
            player.kill();
            mocoParticle.explode(player.sprite.position.x,
                                 player.sprite.position.y,
                                 NK_COLOR_UINT(0xff0000ff), 20);
            gameState = NcGameState::LOSE;
            player.life = 0.0f;
        }

        NcRect door = {tilemap.width / 2, tilemap.height - 30, 20, 30};
        if (NcRect::overlap(door, player.sprite.getRect())) {
            printf("WIN!\n");
            gameState = NcGameState::WIN;
            fadeScreen = 0.0f;
        }
        float explosionRadius = 100.0f;
        for (uint32_t index = 0; index < activeMocoNum; ++index) {
            NcMoco* moco = activeMocos[index];
            if (moco->exploding || !moco->alive)
                continue;
            moco->testTarget(&player.sprite);
            moco->update();
            for (const NcExplosionPoint& point : player.explosionPoints) {
                float centerX =
                    moco->bodyActor.position.x + (moco->bodyActor.width / 2);
                float centerY =
                    moco->bodyActor.position.y + (moco->bodyActor.height / 2);
                float closestX =
                    std::max(moco->bodyActor.position.x,
                             std::min(point.x, moco->bodyActor.position.x +
                                                   moco->bodyActor.width));
                float closestY =
                    std::max(moco->bodyActor.position.y,
                             std::min(point.y, moco->bodyActor.position.y +
                                                   moco->bodyActor.height));
                float distanceX = point.x - closestX;
                float distanceY = point.y - closestY;
                float distanceSquared =
                    (distanceX * distanceX) + (distanceY * distanceY);
                bool hit =
                    distanceSquared <= (explosionRadius * explosionRadius);
                if (hit) {
                    mocoParticle.explode(moco->bodyActor.position.x,
                                         moco->bodyActor.position.y,
                                         NcMoco::COLOR, 10);
                    moco->kill();
                }
            }

            if (moco->alive && moco->exploding) {
                mocoParticle.explode(moco->bodyActor.position.x,
                                     moco->bodyActor.position.y, NcMoco::COLOR,
                                     20);
                moco->kill();
                player.explosionPoints.push_back(NcExplosionPoint{
                    moco->bodyActor.position.x, moco->bodyActor.position.y,
                    NcMoco::COLOR});

                float centerX =
                    moco->bodyActor.position.x + (moco->bodyActor.width / 2);
                float centerY =
                    moco->bodyActor.position.y + (moco->bodyActor.height / 2);
                float closestX = std::max(moco->bodyActor.position.x,
                                          std::min(player.sprite.position.x,
                                                   moco->bodyActor.position.x +
                                                       moco->bodyActor.width));
                float closestY = std::max(moco->bodyActor.position.y,
                                          std::min(player.sprite.position.y,
                                                   moco->bodyActor.position.y +
                                                       moco->bodyActor.height));
                float distanceX = player.sprite.position.x - closestX;
                float distanceY = player.sprite.position.y - closestY;
                float distanceSquared =
                    (distanceX * distanceX) + (distanceY * distanceY);
                bool hit =
                    distanceSquared <= (explosionRadius * explosionRadius);
                if (hit) {
                    player.kill();
                    mocoParticle.explode(player.sprite.position.x,
                                         player.sprite.position.y,
                                         NK_COLOR_UINT(0xff0000ff), 20);
                    moco->target = nullptr;
                    gameState = NcGameState::LOSE;
                }
            }
        }

        for (const NcExplosionPoint& point : player.explosionPoints) {
            float centerX =
                player.sprite.position.x + (player.sprite.width / 2);
            float centerY =
                player.sprite.position.y + (player.sprite.height / 2);
            float closestX =
                std::max(player.sprite.position.x,
                         std::min(point.x, player.sprite.position.x +
                                               player.sprite.width));
            float closestY =
                std::max(player.sprite.position.y,
                         std::min(point.y, player.sprite.position.y +
                                               player.sprite.height));
            float distanceX = point.x - closestX;
            float distanceY = point.y - closestY;
            float distanceSquared =
                (distanceX * distanceX) + (distanceY * distanceY);
            bool hit = distanceSquared <= (80 * 80);
            if (hit) {
                mocoParticle.explode(player.sprite.position.x,
                                     player.sprite.position.y,
                                     NK_COLOR_UINT(0xff0000ff), 10);
                player.kill();
                gameState = NcGameState::LOSE;
            }
        }

    } else {
        if (fadeScreen >= 1.0f) {
            game->switchScene(NcSceneState::MAIN_MENU);
        }
        fadeScreen += 0.5f * game->getElapsedTime();
        if (fadeScreen > 1.0f) {
            fadeScreen = 1.0f;
        }
    }
    mocoParticle.update();
}
void NcGameScene::render() {

    NkCanvas* canvas = game->canvas;
    float followX = player.sprite.position.x - game->width / 2;
    float followY = player.sprite.position.y - game->height / 2;
    followX = std::clamp(followX, 0.0f,
                         tilemap.width - game->width + tilemap.tileWidth);
    followY = std::clamp(followY, 0.0f,
                         tilemap.height - game->height + tilemap.tileHeight);

    if (player.isDigging) {
        followX += (game->randomFloat() * 2.0f - 1.0f);
        followY += (game->randomFloat() * 2.0f - 1.0f);
    }

    if (player.shakeTime > 0.0f) {
        followX += (game->randomFloat() * 2.0f - 1.0f) * 6.0f;
        followY += (game->randomFloat() * 2.0f - 1.0f) * 6.0f;
        player.shakeTime -= game->getElapsedTime();
    }

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, -followX, -followY);
    NcScene::render();
    tilemap.render(followX, followY, game->width, game->height);

    nk::canvas::drawRect(game->canvas, tilemap.width / 2, tilemap.height - 30,
                         20, 30, NK_COLOR_UINT(0x00FFFFff));
    nk::canvas::drawRect(game->canvas, tilemap.width / 2 + 16,
                         tilemap.height - 15, 2, 2, NK_COLOR_UINT(0x000000ff));

    player.render(followX, followY, game->width, game->height);

    mocoParticle.render(0, 0, 0, 0);

    for (uint32_t index = 0; index < activeMocoNum; ++index) {
        activeMocos[index]->render();
    }

    nk::canvas::popMatrix(canvas);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, game->mouse.x, game->mouse.y);
    nk::canvas::rotate(canvas, 0.785398f);
    nk::canvas::drawRect(canvas, -2.0f, -2.0f, 4.0f, 4.0f,
                         NK_COLOR_UINT(0xffff00ff));
    nk::canvas::popMatrix(canvas);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, 8, 8);
    nk::canvas::drawImage(canvas, 0, -4, nigoIcon);
    nk::canvas::drawRect(canvas, nk::img::width(nigoIcon) + 2, 0, 50.0f,
                         nk::img::height(bombIcon), NK_COLOR_UINT(0xffffff55));
    nk::canvas::drawRect(canvas, nk::img::width(nigoIcon) + 2, 0,
                         50.0f * player.life, nk::img::height(bombIcon),
                         NK_COLOR_UINT(0x00ff00ff));
    nk::canvas::popMatrix(canvas);

    nk::canvas::pushMatrix(canvas);
    nk::canvas::translate(canvas, 23, nk::img::height(nigoIcon) + 10);
    nk::canvas::drawImage(canvas, -6, 0, bombIcon);
    nk::canvas::drawRect(canvas, nk::img::width(bombIcon) + 2, 0, 50.0f,
                         nk::img::height(bombIcon), NK_COLOR_UINT(0xffffff55));
    nk::canvas::drawRect(canvas, nk::img::width(bombIcon) + 2, 0,
                         50.0f * (1.0f - (player.bombTimer / 6.0f)),
                         nk::img::height(bombIcon), NK_COLOR_UINT(0xffff00ff));
    nk::canvas::popMatrix(canvas);

    if (player.life < 0.1f) {
        nk::canvas::drawRect(
            canvas, 0, 0, game->width, game->height,
            NK_COLOR_RGBA_FLOAT(1, 0, 0,
                                0.25f * (1.0f - (player.life / 0.1f))));
    }

    if (gameState != NcGameState::PLAYING) {
        if (gameState == NcGameState::WIN) {
            nk::canvas::drawRect(canvas, 0, 0, game->width, game->height,
                                 NK_COLOR_RGBA_FLOAT(1, 1, 1, fadeScreen));
        } else {
            nk::canvas::drawRect(canvas, 0, 0, game->width, game->height,
                                 NK_COLOR_RGBA_FLOAT(1, 0, 0, fadeScreen));
        }
    }
}
