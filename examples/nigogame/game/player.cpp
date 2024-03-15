#include "player.h"
#include "../assets/nigo.h"
#include "assets.h"
#include "game.h"

void NcParticle::render(NcGame* game) {
    nk::canvas::drawRect(game->canvas, position.x, position.y, width, height,
                         color);
}

void NcParticleEmitter::init(NcGame* game) {
    this->game = game;
    pool.init(128);
    for (uint32_t index = 0; index < particles.size(); ++index) {
        NcParticle& particle = particles[index];
        float size = 1 + game->randomFloat() * 4;
        particle.init(game, 0, 0, size, size);
        pool.recycle(&particle);
    }
}
void NcParticleEmitter::destroy() {
    activeParticles.clear();
    pool.destroy();
}
void NcParticleEmitter::update() {
    NcParticle* particlesToRecycle[128] = {};
    uint32_t particlesToRecycleNum = 0;
    for (NcParticle* particle : activeParticles) {
        if (particle->isTouching(NcActor::TOUCH_ANY)) {
            particle->velocity.x *= 0.5f;
        }
        particle->lifetime -= game->getElapsedTime();
        if (particle->lifetime < 0.0f) {
            particlesToRecycle[particlesToRecycleNum++] = particle;
        } else {
            particle->update(game);
        }
    }
    for (uint32_t index = 0; index < particlesToRecycleNum; ++index) {
        for (uint32_t pindex = 0; pindex < activeParticles.size(); ++pindex) {
            if (particlesToRecycle[index] == activeParticles[pindex]) {
                activeParticles.erase(activeParticles.begin() + pindex);
                pool.recycle(particlesToRecycle[index]);
                break;
            }
        }
    }
}
void NcParticleEmitter::render(float viewX, float viewY, float viewWidth,
                               float viewHeight) {
    NcRect view = {viewX, viewY, viewWidth, viewHeight};
    for (NcParticle* particle : activeParticles) {
        // if (NcRect::overlap(view, particle->getRect()))
        particle->render(game);
    }
}
void NcParticleEmitter::explode(float x, float y, uint32_t color,
                                uint32_t count) {
    count = std::min(count, (uint32_t)pool.num);
    NcParticle* newParticles[128] = {};
    for (uint32_t index = 0; index < count; ++index) {
        NcParticle* particle = pool.get();
        particle->lifetime = 2.0f;
        particle->color = color;
        particle->setPosition(x, y);
        particle->acceleration.y = game->getGravity();
        particle->bounce = 0.5f;
        particle->velocity.x = (game->randomFloat() * 2.0f - 1.0f) * 10.0f;
        particle->velocity.y = (game->randomFloat() * 2.0f - 1.0f) * 10.0f;
        activeParticles.push_back(particle);
    }
}

void NcBomb::init(NcPlayer* player, NcGame* game, NkImage* image) {
    sprite.init(game);
    sprite.bounce = 0.6f;
    sprite.acceleration.y = game->getGravity();
    sprite.setImage(image, (uint32_t)nk::img::width(image),
                    (uint32_t)nk::img::height(image));
    sprite.origin = {0.5f, 0.5f};
    life = 2.0f;
    this->player = player;
}
void NcBomb::reset(float x, float y) {
    sprite.setPosition(x, y);
    life = 1.0f;
    sprite.velocity = {0, 0};
}
void NcBomb::update(NcGame* game) {
    if (sprite.isTouching(NcActor::TOUCH_ANY)) {
        sprite.velocity.x *= 0.5f;
    }
    sprite.update(game);
    life -= game->getElapsedTime();
    if (life <= 0.0f) {
        player->recycle(this);
        player->shakeTime = 0.2f;
        player->explosionPoints.push_back(
            {sprite.position.x, sprite.position.y, 0xffffffff});
    }
}
void NcBomb::render(NcGame* game) { sprite.render(game); }

void NcPlayer::init(NcScene* scene, float x, float y) {
    alive = true;
    game = scene->game;
    sprite.init(game, x, y);
    sprite.origin = {0.5f, 1.0f};
    sprite.loadAnimations(scene->addImage("nigo", image_nigo_width,
                                          image_nigo_height, image_nigo),
                          anim_nigo, anim_nigo_num);
    sprite.setAnimationFrameRate("nigo_throw", 13.0f);
    sprite.setAnimationFrameRate("nigo_run", 18.0f);
    sprite.setAnimationFrameRate("nigo_dig", 20.0f);
    sprite.setAnimationFrameRate("nigo_idle", 8.0f);
    sprite.setAnimationFrameRate("nigo_fall", 0.0f);
    sprite.setAnimationFrameRate("nigo_jump", 0.0f);
    sprite.playAnimation("nigo_idle", true);
    sprite.acceleration.y = game->getGravity();
    sprite.width = 15;
    sprite.height = 23;
    isThrowing = false;
    for (uint32_t index = 0; index < 32; ++index) {
        NcBomb bomb{};
        bomb.init(this, game,
                  scene->addImage("bomb", image_bomb_width, image_bomb_height,
                                  image_bomb));
        bombPool[index] = bomb;
        recycledBombs.push_back(&bombPool[index]);
    }
    wasBombThrown = false;
    shakeTime = 0.0f;
    fadeTime = 0.0f;
    explosionImage = scene->addImage("explosion", image_explosion_width,
                                     image_explosion_height, image_explosion);
    bombParticles.init(game);
    digParticles.init(game);
    digParticleDelay = 0.0f;
    activeTile = nullptr;
    isReadyToShoot = false;
    bombTimer = 0.0f;
    life = 1.0f;
}

void NcPlayer::destroy() {
    sprite.destroy(game);
    recycledBombs.clear();
    bombs.clear();
    bombParticles.destroy();
    digParticles.destroy();
    explosionPoints.clear();
    game = nullptr;
}

void NcPlayer::spawnBomb(float x, float y, float angle, float distanceToMouse) {
    if (recycledBombs.size() > 0 && bombTimer <= 0.0f) {
        NcBomb* bomb = recycledBombs[recycledBombs.size() - 1];
        float speedScale = (std::min(distanceToMouse, 200.0f) / 200.0f) * 15.0f;
        recycledBombs.pop_back();
        bomb->reset(x, y - 20.0f);
        bomb->sprite.velocity.x = cosf(angle) * speedScale;
        bomb->sprite.velocity.y = sinf(angle) * speedScale;
        bombs.push_back(bomb);
        wasBombThrown = true;
        bombTimer = 6.0f;
    }
}

void NcPlayer::update(NcTilemap* tilemap) {
    if (!alive)
        return;
    NkHID* hid = game->hid;
    bool wasThrowing = isThrowing && !sprite.isPlaying("nigo_throw");
    bool isPlayingThrow = isThrowing && sprite.isPlaying("nigo_throw");

#if _DEBUG
    if (nk::hid::keyClick(hid, NkKeyCode::CTRL)) {
        sprite.debugDraw = !sprite.debugDraw;
    }

    if (sprite.debugDraw) {
        sprite.velocity = {0, 0};
        sprite.acceleration = {0, 0};
        float speedScale =
            nk::hid::keyDown(hid, NkKeyCode::SHIFT) ? 4.0f : 1.0f;
        if (nk::hid::keyDown(hid, NkKeyCode::A)) {
            sprite.velocity.x = -3.0f * speedScale;
        } else if (nk::hid::keyDown(hid, NkKeyCode::D)) {
            sprite.velocity.x = 3.0f * speedScale;
        }

        if (nk::hid::keyDown(hid, NkKeyCode::W)) {
            sprite.velocity.y = -3.0f * speedScale;
        } else if (nk::hid::keyDown(hid, NkKeyCode::S)) {
            sprite.velocity.y = 3.0f * speedScale;
        }
        sprite.update(game);
        return;
    } else {
        sprite.acceleration.y = game->getGravity();
    }
#endif

    if (bombTimer <= 0.0f && isPlayingThrow && !wasBombThrown &&
        sprite.currentFrameIndex == 2) {
        float viewX = sprite.position.x - game->width / 2;
        float viewY = sprite.position.y - game->height / 2;
        viewX = std::clamp(viewX, 0.0f,
                           tilemap->width - game->width + tilemap->tileWidth);
        viewY = std::clamp(
            viewY, 0.0f, tilemap->height - game->height + tilemap->tileHeight);
        float angle = atan2((viewY + game->mouse.y) - sprite.position.y,
                            (viewX + game->mouse.x) - sprite.position.x);
        float dx = viewX + game->mouse.x - sprite.position.x;
        float dy = viewY + game->mouse.y - sprite.position.y;
        float distanceToMouse = sqrtf(dx * dx + dy * dy);
        spawnBomb(sprite.position.x, sprite.position.y, angle, distanceToMouse);
    }

    if (wasThrowing) {
        isThrowing = false;
        wasBombThrown = false;
    }

    sprite.velocity.x = 0.0f;
    isDigging = false;
    if (!isPlayingThrow && nk::hid::keyDown(hid, NkKeyCode::A) &&
        (!sprite.wasTouching(NcActor::TOUCH_LEFT) ||
         sprite.isTouching(NcActor::TOUCH_FLOOR))) {
        sprite.velocity.x = -3.0f;
        sprite.scale.x = 1.0f;
        if (!isPlayingThrow)
            sprite.playAnimation("nigo_run", true);
    } else if (!isPlayingThrow && nk::hid::keyDown(hid, NkKeyCode::D)) {
        sprite.velocity.x = 3.0f;
        sprite.scale.x = -1.0f;
        if (!isPlayingThrow)
            sprite.playAnimation("nigo_run", true);
    } else if (!isPlayingThrow && sprite.isTouching(NcActor::TOUCH_FLOOR)) {
        if (nk::hid::keyDown(hid, NkKeyCode::S) ||
            nk::hid::mouseDown(hid, NkMouseButton::RIGHT)) {
            sprite.playAnimation("nigo_dig", true);
            isDigging = true;
            if (!activeTile) {
                activeTile =
                    tilemap->getTileAt(sprite.position.x, sprite.position.y);
                for (uint32_t index = 0; index < 4 && !activeTile; ++index) {
                    activeTile = tilemap->getTileAt(
                        sprite.position.x +
                            (game->randomFloat() * 2.0f - 1.0f) * 5,
                        sprite.position.y +
                            (game->randomFloat() * 2.0f - 1.0f) * 5);
                }
                isDigging = !!activeTile;
                if (!isDigging) {
                    sprite.playAnimation("nigo_idle", true);
                }
            }
            if (isDigging && digParticleDelay <= 0.0f) {
                digParticles.explode(sprite.position.x, sprite.position.y,
                                     NK_COLOR_UINT(0x7c5a47ff), 1);
                digParticleDelay = 0.2f;
            }
            if (activeTile && activeTile->alive) {
                activeTile->life -= 0.01f;
                if (activeTile->life < 0.0f) {
                    digParticles.explode(
                        activeTile->position.x + activeTile->width / 2,
                        activeTile->position.y + activeTile->height / 2,
                        NK_COLOR_UINT(0x7c5a47ff), 10);
                    tilemap->explode(activeTile->position.x,
                                     activeTile->position.y, 10);
                    tilemap->killTile(activeTile);
                    activeTile = nullptr;
                    shakeTime = 0.2f;
                }
            }
        } else {
            sprite.playAnimation("nigo_idle", true);
        }
    }

    if (bombTimer <= 0.0f &&
        nk::hid::mouseDown(game->hid, NkMouseButton::LEFT)) {
        float viewX = sprite.position.x - game->width / 2;
        float viewY = sprite.position.y - game->height / 2;
        viewX = std::clamp(viewX, 0.0f,
                           tilemap->width - game->width + tilemap->tileWidth);
        viewY = std::clamp(
            viewY, 0.0f, tilemap->height - game->height + tilemap->tileHeight);
        float dx = viewX + game->mouse.x - sprite.position.x;
        if (dx < 0.0f) {
            sprite.scale.x = 1.0f;
        } else {
            sprite.scale.x = -1.0f;
        }
    }

    if (!isDigging) {
        activeTile = nullptr;
    }

    if (isDigging) {
        digParticleDelay -= game->getElapsedTime();
    } else {
        digParticleDelay = 0.0f;
    }

    if (bombTimer <= 0.0f && !isPlayingThrow &&
        nk::hid::mouseClick(hid, NkMouseButton::LEFT)) {
        isReadyToShoot = true;
    }

    if (bombTimer <= 0.0f && isReadyToShoot &&
        !nk::hid::mouseDown(hid, NkMouseButton::LEFT)) {
        sprite.playAnimation("nigo_throw", false);
        isThrowing = true;
        isPlayingThrow = true;
        isReadyToShoot = false;
    }

    if (sprite.isTouching(NcActor::TOUCH_FLOOR)) {
        if (nk::hid::keyClick(hid, NkKeyCode::W) ||
            nk::hid::keyClick(hid, NkKeyCode::SPACE)) {
            sprite.velocity.y = -8.0f;
        }
    }

    if (!isPlayingThrow) {
        if (sprite.velocity.y < -sprite.acceleration.y * 2) {
            sprite.playAnimation("nigo_jump", false);
        } else if (sprite.velocity.y > sprite.acceleration.y * 2 ||
                   (!sprite.isTouching(NcActor::TOUCH_ANY))) {
            sprite.playAnimation("nigo_fall", false);
        }
    }

    bombParticles.update();
    digParticles.update();
    sprite.update(game);

    for (NcBomb* bomb : bombs) {
        bomb->update(game);
    }

    tilemap->collide(&sprite);
    if (sprite.isTouching(NcActor::TOUCH_LEFT | NcActor::TOUCH_RIGHT)) {
        sprite.playAnimation("idle", true);
    }

    for (NcBomb* bomb : bombs) {
        tilemap->collide(&bomb->sprite);
    }
    for (NcParticle* particle : bombParticles.activeParticles) {
        tilemap->collide(particle);
    }
    for (NcParticle* particle : digParticles.activeParticles) {
        tilemap->collide(particle);
    }
    for (const NcExplosionPoint& point : explosionPoints) {
        if (tilemap->explode(point.x, point.y, 100.0f)) {
            bombParticles.explode(point.x, point.y, NK_COLOR_UINT(0x7c5a47ff),
                                  20);
        }
    }

    if (bombTimer > 0.0f) {
        bombTimer -= game->getElapsedTime();
    }
}

void NcPlayer::render(float viewX, float viewY, float viewWidth,
                      float viewHeight) {

    for (const NcExplosionPoint& point : explosionPoints) {
        nk::canvas::pushMatrix(game->canvas);
        nk::canvas::translate(game->canvas, point.x, point.y);
        nk::canvas::scale(game->canvas, 2.0f, 2.0f);
        nk::canvas::drawImage(game->canvas, -50.f, -50.0f, point.color,
                              explosionImage);
        nk::canvas::popMatrix(game->canvas);
    }
    explosionPoints.clear();
    if (!alive)
        return;
    NcRect view = {viewX, viewY, viewWidth, viewHeight};

    if (bombTimer <= 0.0f &&
        nk ::hid::mouseDown(game->hid, NkMouseButton::LEFT)) {
        float angle = atan2((viewY + game->mouse.y) - sprite.position.y,
                            (viewX + game->mouse.x) - sprite.position.x);
        float mx = sprite.position.x + sprite.width * -sprite.origin.x +
                   cosf(angle) * 50.0f;
        float my = sprite.position.y + sprite.height * -sprite.origin.y +
                   sinf(angle) * 50.0f;

        float dist = 0.0f;
        float dx = viewX + game->mouse.x - sprite.position.x;
        float dy = viewY + game->mouse.y - sprite.position.y;
        float distanceToMouse = std::min(sqrtf(dx * dx + dy * dy), 200.0f);
        float speedScale = (std::min(distanceToMouse, 200.0f) / 200.0f) * 15.0f;
        float px = cosf(angle);
        float py = sinf(angle);

        for (uint32_t index = 0; index < 5; ++index) {
            float mx = sprite.position.x + px * dist;
            float my = sprite.position.y - sprite.height * 0.5f + py * dist;
            float size = (float)(5 - index) + 1;
            nk::canvas::drawRect(game->canvas, mx - (size * 0.5f),
                                 my - (size * 0.5f), size, size,
                                 NK_COLOR_UINT(0xffff00aa));
            dist += speedScale * 2.0f;
        }
    }

    sprite.render(game);
    for (NcBomb* bomb : bombs) {
        if (NcRect::overlap(view, bomb->sprite.getRect()))
            bomb->render(game);
    }
    bombParticles.render(viewX, viewY, viewHeight, viewWidth);
    digParticles.render(viewX, viewY, viewHeight, viewWidth);

    if (activeTile && activeTile->alive && activeTile->life > 0.0f) {
        nk::canvas::drawRect(game->canvas, activeTile->position.x + 2,
                             activeTile->position.y + 2,
                             activeTile->life * activeTile->width - 2, 3,
                             NK_COLOR_UINT(0xffff00ff));
    }
}

void NcPlayer::recycle(NcBomb* bomb) {
    size_t index = 0;
    for (; index < bombs.size(); ++index) {
        if (bomb == bombs[index]) {
            bombs.erase(bombs.begin() + index);
            break;
        }
    }
    recycledBombs.push_back(bomb);
}

void NcPlayer::kill() { alive = false; }