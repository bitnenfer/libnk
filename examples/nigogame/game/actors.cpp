#include "actors.h"
#include "game.h"

/* static */ bool NcRect::pointOn(const NcVector& point, const NcRect& rect) {
    return point.x > rect.x && point.x < rect.x + rect.width &&
           point.y > rect.y && point.y < rect.y + rect.height;
}

/* static */ bool NcRect::overlap(const NcRect& a, const NcRect& b) {
    return a.y + a.height > b.y && a.y < b.y + b.height &&
           a.x + a.width > b.x && a.x < b.x + b.width;
}

/* static */ NcRect NcRect::intersection(const NcRect& a, const NcRect& b) {
    if (overlap(a, b)) {
        float x0 = std::max(a.x, b.x);
        float x1 = std::min(a.x + a.width, b.x + b.width);
        float y0 = std::max(a.y, b.y);
        float y1 = std::min(a.y + a.height, b.y + b.height);
        return {x0, y0, x1 - x0, y1 - y0};
    }
    return {0, 0, 0, 0};
}

/* static */ bool NcRect::collide(const NcRect& dynamicRect,
                                  const NcRect& staticRect, NcVector& output) {
    NcRect rect = NcRect::intersection(dynamicRect, staticRect);
    if (rect.getArea() > 0) {
        if (rect.width > rect.height) {
            float sign =
                std::signbit(staticRect.y - dynamicRect.y) ? 1.0f : -1.0f;
            output.y = rect.height * sign;
        } else {
            float sign =
                std::signbit(staticRect.x - dynamicRect.x) ? 1.0f : -1.0f;
            output.x = rect.width * sign;
        }
        return true;
    }
    return false;
}

void NcActor::init(NcGame* game, float x, float y, float inwidth,
                   float inheight) {
    position = {x, y};
    last = position;
    velocity = {0, 0};
    acceleration = {0, 0};
    scale = {1, 1};
    rotation = 0.0f;
    bounce = 0.0f;
    width = inwidth;
    height = inheight;
    touching = NcActor::TOUCH_ANY;
    debugDraw = false;
}
void NcActor::destroy(NcGame* game) {}
void NcActor::update(NcGame* game) {
    float elapsedTime = game->getElapsedTime();
    last = position;
    velocity += acceleration;
    constexpr float maxVelocity = 1000.0f;
    if (velocity.x > maxVelocity) {
        velocity.x = maxVelocity;
    } else if (velocity.x < -maxVelocity) {
        velocity.x = -maxVelocity;
    }
    if (velocity.y > maxVelocity) {
        velocity.y = maxVelocity;
    } else if (velocity.y < -maxVelocity) {
        velocity.y = -maxVelocity;
    }
    position += velocity;
    touchingPrev = touching;
    touching = NcActor::TOUCH_NONE;
}
void NcActor::render(NcGame* game) {
    if (debugDraw) {
        NkCanvas* canvas = game->canvas;
        NcRect rect = getRect();
        nk::canvas::drawRect(canvas, rect.x, rect.y, rect.width, rect.height,
                             NK_COLOR_RGBA_FLOAT(1, 1, 1, 0.25f));
    }
}

NcRect NcActor::getRect() const {
    return {position.x, position.y, width, height};
}

NcBodyAABB NcActor::getBodyAABB() const {
    return {position, last, width, height};
}

/* static */ bool NcActor::separateX(NcActor* dynamicActor,
                                     NcActor* staticActor) {
    NcBodyAABB aabb0 = dynamicActor->getBodyAABB();
    NcBodyAABB aabb1 = staticActor->getBodyAABB();
    NcVector last0 = aabb0.last;
    NcVector last1 = aabb1.last;
    float totalOverlap = 0.0f;
    float delta0 = aabb0.position.x - last0.x;
    float delta1 = aabb1.position.x - last1.x;
    if (delta0 != delta1) {
        float delta0Abs = fabsf(delta0);
        float delta1Abs = fabsf(delta1);
        NcRect rect0 = {aabb0.position.x - std::max(delta0, 0.0f), last0.y,
                        aabb0.width + delta0Abs, aabb0.height};
        NcRect rect1 = {aabb1.position.x - std::max(delta1, 0.0f), last1.y,
                        aabb1.width + delta1Abs, aabb1.height};
        if (rect0.x + rect0.width > rect1.x &&
            rect0.x < rect1.x + rect1.width &&
            rect0.y + rect0.height > rect1.y &&
            rect0.y < rect1.y + rect1.height) {
            float maxOverlap = delta0Abs + delta1Abs + 4.0f;
            if (delta0 > delta1) {
                totalOverlap =
                    aabb0.position.x + aabb0.width - aabb1.position.x;
                if (totalOverlap > maxOverlap) {
                    totalOverlap = 0.0f;
                } else {
                    dynamicActor->touching |= NcActor::TOUCH_RIGHT;
                    staticActor->touching |= NcActor::TOUCH_LEFT;
                }
            } else if (delta0 < delta1) {
                totalOverlap =
                    aabb0.position.x - aabb1.width - aabb1.position.x;
                if (-totalOverlap > maxOverlap) {
                    totalOverlap = 0.0f;
                } else {
                    dynamicActor->touching |= NcActor::TOUCH_LEFT;
                    staticActor->touching |= NcActor::TOUCH_RIGHT;
                }
            }
        }
    }

    if (totalOverlap != 0.0f) {
        float v0 = dynamicActor->velocity.x;
        float v1 = staticActor->velocity.y;
        dynamicActor->position.x -= totalOverlap;
        dynamicActor->velocity.x = v1 - v0 * dynamicActor->bounce;
        return true;
    }

    return false;
}

/* static */ bool NcActor::separateY(NcActor* dynamicActor,
                                     NcActor* staticActor) {
    NcBodyAABB aabb0 = dynamicActor->getBodyAABB();
    NcBodyAABB aabb1 = staticActor->getBodyAABB();
    NcVector last0 = aabb0.last;
    NcVector last1 = aabb1.last;
    float totalOverlap = 0.0f;
    float delta0 = aabb0.position.y - last0.y;
    float delta1 = aabb1.position.y - last1.y;
    if (delta0 != delta1) {
        float delta0Abs = fabsf(delta0);
        float delta1Abs = fabsf(delta1);
        NcRect rect0 = {aabb0.position.x,
                        aabb0.position.y - std::max(delta0, 0.0f), aabb0.width,
                        aabb0.height + delta0Abs};
        NcRect rect1 = {aabb1.position.x,
                        aabb1.position.y - std::max(delta1, 0.0f), aabb1.width,
                        aabb1.height + delta1Abs};

        if (rect0.x + rect0.width > rect1.x &&
            rect0.x < rect1.x + rect1.width &&
            rect0.y + rect0.height > rect1.y &&
            rect0.y < rect1.y + rect1.height) {
            float maxOverlap = delta0Abs + delta1Abs + 4.0f;
            if (delta0 > delta1) {
                totalOverlap =
                    aabb0.position.y + aabb0.height - aabb1.position.y;
                if (totalOverlap > maxOverlap) {
                    totalOverlap = 0.0f;
                } else {
                    dynamicActor->touching |= NcActor::TOUCH_DOWN;
                    staticActor->touching |= NcActor::TOUCH_UP;
                }
            } else if (delta0 < delta1) {
                totalOverlap =
                    aabb0.position.y - aabb1.height - aabb1.position.y;
                if (-totalOverlap > maxOverlap) {
                    totalOverlap = 0.0f;
                } else {
                    dynamicActor->touching |= NcActor::TOUCH_UP;
                    staticActor->touching |= NcActor::TOUCH_DOWN;
                }
            }
        }
    }

    if (totalOverlap != 0.0f) {
        float v0 = dynamicActor->velocity.y;
        float v1 = staticActor->velocity.y;
        dynamicActor->position.y -= totalOverlap;
        dynamicActor->velocity.y = v1 - v0 * dynamicActor->bounce;
        if (staticActor->velocity.y != 0.0f && delta0 > delta1) {
            dynamicActor->position.x += aabb1.position.x - last1.x;
        }
        return true;
    }
    return false;
}

/* static */ bool NcActor::collide(NcActor* dynamicActor,
                                   NcActor* staticActor) {
    // return NcActor::separateY(dynamicActor, staticActor) ||
    //        NcActor::separateX(dynamicActor, staticActor);

    NcRect rect1 = dynamicActor->getRect();
    NcRect rect2 = staticActor->getRect();

    if (NcRect::overlap(rect1, rect2)) {
        NcRect intersection = NcRect::intersection(rect1, rect2);
        if (intersection.width < intersection.height) {
            if (rect1.x < rect2.x) {
                dynamicActor->position.x -= intersection.width;
                dynamicActor->last.x -= intersection.width;
                dynamicActor->touching |= NcActor::TOUCH_RIGHT;
            } else {
                dynamicActor->position.x += intersection.width;
                dynamicActor->last.x += intersection.width;
                dynamicActor->touching |= NcActor::TOUCH_LEFT;
            }
            dynamicActor->velocity.x = 0.0f;
        }
        return NcActor::separateX(dynamicActor, staticActor) ||
               NcActor::separateY(dynamicActor, staticActor);
    }
    return false;
}

/* static */ bool NcActor::overlap(const NcActor* a, const NcActor* b) {
    return NcRect::overlap(a->getRect(), b->getRect());
}

void NcSprite::init(NcGame* game, float x, float y, float width, float height) {
    NcActor::init(game, x, y, width, height);
    currentAnimation = nullptr;
    currentFrame = {};
    image = nullptr;
    currentFrameIndex = 0;
    frameWidth = 0.0f;
    frameHeight = 0.0f;
    animTime = 0.0f;
    animLooped = false;
    animPlaying = false;
    animations.clear();
    frames.clear();
    origin = {0.0f, 0.0f};
    color = 0xFFFFFFFF;
    visible = true;
}
void NcSprite::destroy(NcGame* game) {
    NcActor::destroy(game);
    animations.clear();
    frames.clear();
    image = nullptr;
}
void NcSprite::update(NcGame* game) {
    NcActor::update(game);
    if (animPlaying && currentAnimation) {
        animTime += game->getElapsedTime();
        if (animTime >= currentAnimation->frameRate) {
            if (animLooped) {
                currentFrameIndex =
                    (currentFrameIndex + 1) % currentAnimation->frames.size();
                currentFrame =
                    frames[currentAnimation->frames[currentFrameIndex]];
            } else if (currentFrameIndex + 1 <
                       currentAnimation->frames.size()) {
                currentFrameIndex++;
                currentFrame =
                    frames[currentAnimation->frames[currentFrameIndex]];
            } else {
                animPlaying = false;
            }
            animTime = 0.0f;
        }
    }
}
void NcSprite::render(NcGame* game) {
    NcActor::render(game);
    if (image != nullptr && visible) {
        NkCanvas* canvas = game->canvas;
        nk::canvas::pushMatrix(canvas);
        nk::canvas::translate(canvas, position.x, position.y);
        nk::canvas::rotate(canvas, rotation);
        nk::canvas::scale(canvas, scale.x, scale.y);
        nk::canvas::drawImage(
            canvas, -currentFrame.width * origin.x,
            -currentFrame.height * origin.y, currentFrame.width,
            currentFrame.height, currentFrame.x, currentFrame.y,
            currentFrame.width, currentFrame.height, color, image);
        nk::canvas::popMatrix(canvas);
    }
}
NcRect NcSprite::getRect() const {
    return {position.x - (width * origin.x), position.y - (height * origin.y),
            width, height};
}

NcBodyAABB NcSprite::getBodyAABB() const {
    NcVector offset = {-(width * origin.x), -(height * origin.y)};
    return {position + offset, last + offset, width, height};
}

void NcSprite::setImage(NkImage* image, uint32_t frameWidth,
                        uint32_t frameHeight) {
    float imageWidth = nk::img::width(image);
    float imageHeight = nk::img::height(image);
    uint32_t frameCount = (uint32_t)ceilf((imageWidth * imageHeight) /
                                          (frameWidth * frameHeight));
    float ix = 0.0f;
    float iy = 0.0f;
    for (uint32_t index = 0; index < frameCount; ++index) {
        if (ix + frameWidth > imageWidth) {
            ix = 0.0f;
            iy += frameHeight;
        }
        frames.push_back({ix, iy, (float)frameWidth, (float)frameHeight});
        ix += frameWidth;
    }
    this->image = image;
    this->frameWidth = (float)frameWidth;
    this->frameHeight = (float)frameHeight;
    currentFrame = {0.0f, 0.0f, (float)frameWidth, (float)frameHeight};
    width = (float)frameWidth;
    height = (float)frameHeight;
}
void NcSprite::addAnimation(const std::string& animName,
                            const std::vector<uint32_t>& frames,
                            float frameRate) {
    if (animations.find(animName) == animations.end()) {
        animations.insert({animName, {frames, 1.0f / frameRate}});
    }
}
void NcSprite::playAnimation(const std::string& animName, bool loop) {
    const auto& animation = animations.find(animName);
    if (animation != animations.end() &&
        currentAnimation != &animation->second) {
        currentAnimation = &animation->second;
        currentFrameIndex = 0;
        animLooped = loop;
        animPlaying = true;
        animTime = 0.0f;
        currentFrame = frames[currentAnimation->frames[0]];
        currentAnimationName = animName;
    }
}
void NcSprite::stopAnimation() { animPlaying = false; }

bool NcSprite::isPlaying(const std::string& name) const {
    return animPlaying && currentAnimationName == name;
}

void NcSprite::loadAnimations(NkImage* image,
                              const NcSpritesheetAnimation* animations,
                              uint32_t animationNum) {
    frames.clear();
    this->animations.clear();
    this->image = image;
    for (uint32_t index = 0; index < animationNum; ++index) {
        const NcSpritesheetAnimation& anim = animations[index];
        const uint32_t offset = (uint32_t)frames.size();
        for (uint32_t frameIndex = 0; frameIndex < anim.frameCount;
             ++frameIndex) {
            NcAnimationFrame frame{};
            frame.x = anim.frames[frameIndex * 4 + 0];
            frame.y = anim.frames[frameIndex * 4 + 1];
            frame.width = anim.frames[frameIndex * 4 + 2];
            frame.height = anim.frames[frameIndex * 4 + 3];
            frames.push_back(frame);
        }
        std::vector<uint32_t> frames;
        for (uint32_t frameIndex = 0; frameIndex < anim.frameCount;
             ++frameIndex) {
            frames.push_back(offset + frameIndex);
        }
        this->animations.insert({anim.name, {frames, 0.0f}});
    }
}

void NcSprite::setAnimationFrameRate(const std::string& name, float frameRate) {
    const auto& animation = animations.find(name);
    if (animation != animations.end()) {
        animation->second.frameRate = 1.0f / frameRate;
    }
}
