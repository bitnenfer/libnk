#pragma once

#include <assert.h>
#include <nk/canvas.h>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T> struct NcPool {

    inline void init(size_t capacity) {
        max = capacity;
        num = 0;
        pool = (T**)calloc(capacity, sizeof(T*));
    }
    inline void destroy() { free(pool); }
    inline void recycle(T* element) {
        if (num + 1 <= max) {
            pool[num++] = element;
        } else {
            assert(0);
        }
    }
    inline T* get() {
        if (num > 0) {
            return pool[--num];
        }
        assert(0);
        return nullptr;
    }

    T** pool;
    size_t num;
    size_t max;
};

struct NcGame;

struct NcSpritesheetAnimation {
    const char* name;
    unsigned int frameCount;
    const float* frames;
};

struct NcVector {

    NcVector(float x, float y) : x(x), y(y) {}
    NcVector() : x(0), y(0) {}
    NcVector(const NcVector& vec) : x(vec.x), y(vec.y) {}

    friend bool operator!=(const NcVector& a, const NcVector& b) {
        return a.x != b.x || a.y != b.y;
    }

    friend bool operator==(const NcVector& a, const NcVector& b) {
        return a.x == b.x && a.y == b.y;
    }

    friend NcVector operator+(const NcVector& a, const NcVector& b) {
        return {a.x + b.x, a.y + b.y};
    }

    inline NcVector& operator=(const NcVector& vec) {
        x = vec.x;
        y = vec.y;
        return *this;
    }

    inline NcVector& operator+=(const NcVector& vec) {
        x += vec.x;
        y += vec.y;
        return *this;
    }

    inline NcVector& operator-=(const NcVector& vec) {
        x -= vec.x;
        y -= vec.y;
        return *this;
    }

    inline NcVector& operator*=(const NcVector& vec) {
        x *= vec.x;
        y *= vec.y;
        return *this;
    }

    inline NcVector& operator/=(const NcVector& vec) {
        x /= vec.x;
        y /= vec.y;
        return *this;
    }

    inline NcVector& operator*=(float value) {
        x *= value;
        y *= value;
        return *this;
    }

    inline NcVector operator+(const NcVector& vec) {
        NcVector output = *this;
        output.x += vec.x;
        output.y += vec.y;
        return output;
    }

    inline NcVector operator-(const NcVector& vec) {
        NcVector output = *this;
        output.x -= vec.x;
        output.y -= vec.y;
        return output;
    }

    inline NcVector operator*(const NcVector& vec) {
        NcVector output = *this;
        output.x *= vec.x;
        output.y *= vec.y;
        return output;
    }

    inline NcVector operator/(const NcVector& vec) {
        NcVector output = *this;
        output.x /= vec.x;
        output.y /= vec.y;
        return output;
    }

    inline NcVector operator*(float value) {
        NcVector output = *this;
        output.x *= value;
        output.y *= value;
        return output;
    }

    float x;
    float y;
};

struct NcRect {

    static bool pointOn(const NcVector& point, const NcRect& rect);
    static bool overlap(const NcRect& a, const NcRect& b);
    static NcRect intersection(const NcRect& a, const NcRect& b);
    static bool collide(const NcRect& dynamicRect, const NcRect& staticRect,
                        NcVector& output);

    inline float getArea() const { return width * height; }

    float x;
    float y;
    float width;
    float height;
};

struct NcBodyAABB {
    NcVector position;
    NcVector last;
    float width;
    float height;
};

struct NcActor {

    static constexpr uint32_t TOUCH_NONE = 0b0000;
    static constexpr uint32_t TOUCH_UP = 0b0001;
    static constexpr uint32_t TOUCH_DOWN = 0b0010;
    static constexpr uint32_t TOUCH_LEFT = 0b0100;
    static constexpr uint32_t TOUCH_RIGHT = 0b1000;
    static constexpr uint32_t TOUCH_ANY =
        TOUCH_UP | TOUCH_DOWN | TOUCH_LEFT | TOUCH_RIGHT;

    static constexpr uint32_t TOUCH_CEILING = TOUCH_UP;
    static constexpr uint32_t TOUCH_FLOOR = TOUCH_DOWN;

    virtual void init(NcGame* game, float x = 0.0f, float y = 0.0f,
                      float width = 0.0f, float height = 0.0f);
    virtual void destroy(NcGame* game);
    virtual void update(NcGame* game);
    virtual void render(NcGame* game);
    virtual NcRect getRect() const;
    virtual NcBodyAABB getBodyAABB() const;

    // flixel's collision
    bool isTouching(uint32_t value) const { return (touching & value) > 0; }
    bool wasTouching(uint32_t value) const {
        return (touchingPrev & value) > 0;
    }
    inline void setPosition(float x, float y) {
        position = {x, y};
        last = position;
    }

    static bool separateX(NcActor* dynamicActor, NcActor* staticActor);
    static bool separateY(NcActor* dynamicActor, NcActor* staticActor);
    static bool collide(NcActor* dynamicActor, NcActor* staticActor);
    static bool overlap(const NcActor* a, const NcActor* b);

    NcVector last;
    NcVector position;
    NcVector velocity;
    NcVector acceleration;
    NcVector scale;
    float rotation;
    float bounce;
    float width;
    float height;
    uint32_t touching;
    uint32_t touchingPrev;
    bool debugDraw;
};

struct NcAnimationFrame {
    float x;
    float y;
    float width;
    float height;
};

struct NcAnimation {
    std::vector<uint32_t> frames;
    float frameRate;
};

struct NcSprite : public NcActor {

    virtual void init(NcGame* game, float x = 0.0f, float y = 0.0f,
                      float width = 0.0f, float height = 0.0f) override;
    virtual void destroy(NcGame* game) override;
    virtual void update(NcGame* game) override;
    virtual void render(NcGame* game) override;
    virtual NcRect getRect() const override;
    virtual NcBodyAABB getBodyAABB() const override;

    void setImage(NkImage* image, uint32_t frameWidth, uint32_t frameHeight);
    void addAnimation(const std::string& animName,
                      const std::vector<uint32_t>& frames,
                      float frameRate = 0.0f);
    void playAnimation(const std::string& animName, bool loop);
    void stopAnimation();
    bool animationFinished() const { return animPlaying; }
    void loadAnimations(NkImage* image,
                        const NcSpritesheetAnimation* animations,
                        uint32_t animationNum);
    void setAnimationFrameRate(const std::string& name, float frameRate);
    bool isPlaying(const std::string& name) const;

public:
    NcVector origin;
    uint32_t color;
    bool visible;
    NcAnimationFrame currentFrame;
    std::string currentAnimationName;
    uint32_t currentFrameIndex;

private:
    std::unordered_map<std::string, NcAnimation> animations;
    std::vector<NcAnimationFrame> frames;
    NcAnimation* currentAnimation;
    NkImage* image;
    float frameWidth;
    float frameHeight;
    float animTime;
    bool animLooped;
    bool animPlaying;
};
