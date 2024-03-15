#pragma once

#include "actors.h"
#include "memory.h"

struct NcGame;

#define NC_MAX_ELEMENTS_PER_CELL (128 * 4)
struct NcSpatialPartitionCell : public NcRect {

    void init(float x, float y, float width, float height);
    void destroy();
    bool collide(NcActor* actor);
    void insert(NcActor* actor);
    void render(NkCanvas* canvas);
    void remove(NcActor* actor);

    NcActor* actors[NC_MAX_ELEMENTS_PER_CELL];
    uint32_t actorNum;
};

struct NcSpatialPartitionGrid {

    void init(float x, float y, uint32_t columnNum, uint32_t rowNum,
              uint32_t cellWidth, uint32_t cellHeight);
    void destroy();
    bool collide(NcActor* actor);
    bool getCells(NcRect rect, NcSpatialPartitionCell** outputCells,
                  uint32_t* outputCellNum);
    void insert(NcActor* actor);
    void remove(NcActor* actor);
    void render(NkCanvas* canvas);

    NcRect root;
    NcSpatialPartitionCell* cells;
    uint32_t cellNum;
};

struct NcTile : public NcSprite {

    virtual void render(NcGame* game);

    float life;
    uint32_t id;
    uint32_t uid;
    bool alive;
    float colorOffset;
};

struct NcTilemap {

    void init(NcGame* game);
    void destroy();
    void generate(uint32_t width, uint32_t height);
    void collide(NcActor* actor);
    void render(float viewX, float viewY, float viewWidth, float viewHeight);
    bool explode(float x, float y, float radius);
    void killTile(NcTile* tile);
    void addTile(NcTile* tile, uint32_t id);
    NcTile* getTileAt(float x, float y);

    NcSpatialPartitionGrid collisionGrid;
    std::vector<NcTile*> activeTiles;
    NcObjectAllocator<NcTile> tileAllocator;
    NcGame* game;
    uint32_t uidCount;
    float width;
    float height;
    float tileWidth;
    float tileHeight;
};
