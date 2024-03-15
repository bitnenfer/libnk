#include "tilemap.h"
#include "game.h"

void NcSpatialPartitionCell::init(float x, float y, float width, float height) {
    this->x = x;
    this->y = y;
    this->width = width;
    this->height = height;
}

void NcSpatialPartitionCell::destroy() {}

bool NcSpatialPartitionCell::collide(NcActor* actor) {
    uint32_t collideCount = 0;
    for (uint32_t index = 0; index < actorNum; ++index) {
        if (NcActor::collide(actor, actors[index])) {
            collideCount++;
        }
    }
    return collideCount > 0;
}

void NcSpatialPartitionCell::insert(NcActor* actor) {
    if (actorNum + 1 <= NC_MAX_ELEMENTS_PER_CELL) {
        actors[actorNum++] = actor;
        return;
    }
    assert(0);
}

void NcSpatialPartitionCell::remove(NcActor* actor) {
    for (uint32_t index = 0; index < actorNum; ++index) {
        if (actor == actors[index]) {
            if (actorNum - index > 1) {
                actors[index] = actors[actorNum - 1];
            }
            actorNum--;
            return;
        }
    }
}

void NcSpatialPartitionCell::render(NkCanvas* canvas) {
    nk::canvas::drawRect(canvas, x + 1, y + 1, width - 1, height - 1,
                         NK_COLOR_RGBA_FLOAT(1, 1, 1, 0.2f));
}

void NcTile::render(NcGame* game) {
    nk::canvas::drawRect(game->canvas, position.x, position.y, width, height,
                         NK_COLOR_RGB_FLOAT(0.4862f + colorOffset,
                                            0.3529f + colorOffset,
                                            0.2784f + colorOffset));
}

void NcSpatialPartitionGrid::init(float x, float y, uint32_t columnNum,
                                  uint32_t rowNum, uint32_t cellWidth,
                                  uint32_t cellHeight) {
    assert(columnNum > 0 && rowNum > 0 && cellWidth > 0 && cellHeight > 0);
    root = {x, y, (float)(cellWidth * columnNum), (float)(cellHeight * rowNum)};
    cells = (NcSpatialPartitionCell*)malloc(columnNum * rowNum *
                                            sizeof(NcSpatialPartitionCell));
    cellNum = 0;
    for (uint32_t rindex = 0; rindex < rowNum; ++rindex) {
        for (uint32_t cindex = 0; cindex < columnNum; ++cindex) {
            NcSpatialPartitionCell cell{};
            cell.init(x + (float)(cindex * cellWidth),
                      y + (float)(rindex * cellHeight), (float)cellWidth,
                      (float)cellHeight);
            cells[cellNum++] = cell;
        }
    }
}
void NcSpatialPartitionGrid::destroy() {
    if (cells) {
        for (uint32_t index = 0; index < cellNum; ++index) {
            cells[index].destroy();
        }
        free(cells);
        cells = nullptr;
    }
}
bool NcSpatialPartitionGrid::collide(NcActor* actor) {
    NcSpatialPartitionCell* activeCells[4] = {};
    uint32_t activeCellNum = 0;
    uint32_t collided = 0;
    if (getCells(actor->getRect(), activeCells, &activeCellNum)) {
        for (uint32_t index = 0; index < activeCellNum; ++index) {
            if (activeCells[index]->collide(actor)) {
                collided++;
            }
        }
    }
    return collided > 0;
}
bool NcSpatialPartitionGrid::getCells(NcRect rect,
                                      NcSpatialPartitionCell** outputCells,
                                      uint32_t* outputCellNum) {
    if (NcRect::overlap(root, rect)) {
        uint32_t count = 0;
        for (uint32_t index = 0; index < cellNum; ++index) {
            if (NcRect::overlap(cells[index], rect)) {
                outputCells[count++] = &cells[index];
            }
        }
        *outputCellNum = count;
        return count > 0;
    }
    return false;
}
void NcSpatialPartitionGrid::insert(NcActor* actor) {
    NcSpatialPartitionCell* activeCells[4] = {};
    uint32_t activeCellNum = 0;
    if (getCells(actor->getRect(), activeCells, &activeCellNum)) {
        for (uint32_t index = 0; index < activeCellNum; ++index) {
            activeCells[index]->insert(actor);
        }
    }
}
void NcSpatialPartitionGrid::remove(NcActor* actor) {
    NcSpatialPartitionCell* activeCells[4] = {};
    uint32_t activeCellNum = 0;
    if (getCells(actor->getRect(), activeCells, &activeCellNum)) {
        for (uint32_t index = 0; index < activeCellNum; ++index) {
            activeCells[index]->remove(actor);
        }
    }
}
void NcSpatialPartitionGrid::render(NkCanvas* canvas) {
    for (uint32_t index = 0; index < cellNum; ++index) {
        cells[index].render(canvas);
    }
}

void NcTilemap::init(NcGame* game) {
    this->game = game;
    width = 0;
    height = 0;
    tileWidth = 40.0f;
    tileHeight = 40.0f;
    tileAllocator.init(1 << 18);
    uidCount = 0;
}
void NcTilemap::destroy() {
    collisionGrid.destroy();
    activeTiles.clear();
    tileAllocator.destroy();
}
void NcTilemap::generate(uint32_t width, uint32_t height) {
    activeTiles.clear();

    collisionGrid.init(0, 0, (uint32_t)(width * tileWidth) / 50 + 1,
                       (uint32_t)(height * tileHeight) / 50 + 1, 100, 100);

    this->width = width * tileWidth;
    this->height = height * tileHeight;
    // generate walls
    for (uint32_t x = 0; x < width + 1; ++x) {
        NcTile* top = tileAllocator.create();
        top->init(game, tileWidth * x, 0, tileWidth, tileHeight);

        NcTile* bottom = tileAllocator.create();
        bottom->init(game, tileWidth * x, tileHeight * height, tileWidth,
                     tileHeight);
        addTile(top, 1);
        addTile(bottom, 1);
    }

    for (uint32_t y = 0; y < height; ++y) {
        NcTile* left = tileAllocator.create();
        left->init(game, 0, tileHeight * y, tileWidth, tileHeight);
        NcTile* right = tileAllocator.create();
        right->init(game, tileWidth * width, tileHeight * y, tileWidth,
                    tileHeight);
        addTile(left, 1);
        addTile(right, 1);
    }

    struct NcCaveHole {
        float x;
        float y;
        float radius;
    };

    // The worst way of creating caves :D
    NcCaveHole holes[512] = {};
    uint32_t holeNum = 2;
    holes[0] = {game->width / 2.0f, 100.0f, 150.0f};
    holes[1] = {game->width / 2.0f, height * tileHeight, 150.0f};
    for (uint32_t index = 2; index < 300; ++index) {
        holes[holeNum++] = {game->randomFloat() * this->width, index * 40.0f,
                            85.0f};
    }

    float tileWidthHalf = tileWidth / 2;
    float tileHeightHalf = tileHeight / 2;
    for (uint32_t y = 1; y < height; ++y) {
        for (uint32_t x = 1; x < width; ++x) {
            float tx = tileWidth * x;
            float ty = tileHeight * y;
            bool shouldCreate = true;
            for (uint32_t index = 0; index < holeNum; ++index) {
                NcCaveHole& hole = holes[index];
                float centerX = tx + tileWidthHalf;
                float centerY = ty + tileHeightHalf;
                float closestX = std::max(tx, std::min(hole.x, tx + tileWidth));
                float closestY =
                    std::max(ty, std::min(hole.y, ty + tileHeight));
                float distanceX = hole.x - closestX;
                float distanceY = hole.y - closestY;
                float distanceSquared =
                    (distanceX * distanceX) + (distanceY * distanceY);
                bool hit = distanceSquared <= (hole.radius * hole.radius);

                if (hit) {
                    shouldCreate = false;
                    break;
                }
            }
            if (shouldCreate) {
                NcTile* tile = tileAllocator.create();
                tile->init(game, tx, ty, tileWidth, tileHeight);
                addTile(tile, 2);
            }
        }
    }
}
void NcTilemap::collide(NcActor* actor) { collisionGrid.collide(actor); }

void NcTilemap::render(float viewX, float viewY, float viewWidth,
                       float viewHeight) {
    NcRect view = {viewX, viewY, viewWidth, viewHeight};
    for (NcTile* tile : activeTiles) {
        if (tile->alive && NcRect::overlap(view, tile->getRect()))
            tile->render(game);
    }
}
bool NcTilemap::explode(float x, float y, float radius) {
    bool result = false;
    for (NcTile* tile : activeTiles) {
        if (tile->id < 2 || !tile->alive)
            continue;
        float centerX = tile->position.x + (tile->width / 2);
        float centerY = tile->position.y + (tile->height / 2);
        float closestX = std::max(tile->position.x,
                                  std::min(x, tile->position.x + tile->width));
        float closestY = std::max(tile->position.y,
                                  std::min(y, tile->position.y + tile->height));
        float distanceX = x - closestX;
        float distanceY = y - closestY;
        float distanceSquared =
            (distanceX * distanceX) + (distanceY * distanceY);
        bool hit = distanceSquared <= (radius * radius);

        if (hit) {
            killTile(tile);
            result = true;
        }
    }
    return result;
}

NcTile* NcTilemap::getTileAt(float x, float y) {
    const float radius = 1.0f;
    for (NcTile* tile : activeTiles) {
        if (tile->id < 2 || !tile->alive)
            continue;
        float centerX = tile->position.x + (tile->width / 2);
        float centerY = tile->position.y + (tile->height / 2);
        float closestX = std::max(tile->position.x,
                                  std::min(x, tile->position.x + tile->width));
        float closestY = std::max(tile->position.y,
                                  std::min(y, tile->position.y + tile->height));
        float distanceX = x - closestX;
        float distanceY = y - closestY;
        float distanceSquared =
            (distanceX * distanceX) + (distanceY * distanceY);
        bool hit = distanceSquared <= (radius * radius);
        if (hit) {
            return tile;
        }
    }
    return nullptr;
}

void NcTilemap::killTile(NcTile* tile) {
    if (!tile->alive)
        return;
    for (uint32_t tindex = 0; tindex < activeTiles.size(); ++tindex) {
        if (tile == activeTiles[tindex]) {
            collisionGrid.remove(tile);
            activeTiles[tindex]->alive = false;
            break;
        }
    }
}

void NcTilemap::addTile(NcTile* tile, uint32_t id) {
    tile->alive = true;
    tile->id = id;
    tile->uid = uidCount++;
    tile->life = 1.0f;
    tile->colorOffset = (game->randomFloat() * 2.0f - 1.0f) * 0.1f;

    activeTiles.push_back(tile);
    collisionGrid.insert(tile);
}
