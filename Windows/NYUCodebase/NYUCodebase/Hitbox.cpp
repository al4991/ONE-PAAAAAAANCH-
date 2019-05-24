#include "helper.h"
#include "Hitbox.h"
#include <algorithm>


bool Hitbox::checkFulfill(FlareMap* map) {
    int gridX;
    int gridY;

    //top
    worldToTileCoordinates(body.x, body.y + 0.5f * body.height, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            return true;
        }
    }
    //bottom
    worldToTileCoordinates(body.x, body.y - 0.5f * body.height, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            return true;
        }
    }

    //left
    worldToTileCoordinates(body.x - 0.5f * body.width, body.y, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            return true;
        }
    }

    //right
    worldToTileCoordinates(body.x + 0.5f * body.width, body.y, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            return true;
        }
    }

    return false;
}