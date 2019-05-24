#include "Entity.h"
#include "helper.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>

Entity::Entity() {}

Entity::Entity(float x_, float y_, float width_, float height_, bool isStatic_) {
    x = x_;
    y = y_;
    width = width_;
    height = height_;
    isStatic = isStatic_;
}

void Entity::Render(ShaderProgram& p) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
    p.SetModelMatrix(modelMatrix);
    float u; 
    float v;
    if (spriteSet == 0) {
        u = (float)(((int)sprites[spriteIndex]) % sheet.spriteCountX) / (float)sheet.spriteCountX;
        v = (float)(((int)sprites[spriteIndex]) / sheet.spriteCountX) / (float)sheet.spriteCountY;

    }
    else if (spriteSet == 1) {
        u = (float)(((int)forwardSprites[spriteIndex]) % sheet.spriteCountX) / (float)sheet.spriteCountX;
        v = (float)(((int)forwardSprites[spriteIndex]) / sheet.spriteCountX) / (float)sheet.spriteCountY;

    }		
    else if (spriteSet == 2) {
        u = (float)(((int)backwardSprites[spriteIndex]) % sheet.spriteCountX) / (float)sheet.spriteCountX;
        v = (float)(((int)backwardSprites[spriteIndex]) / sheet.spriteCountX) / (float)sheet.spriteCountY;
    }

    float spriteWidth = 1.0f / (float)sheet.spriteCountX;
    float spriteHeight = 1.0f / (float)sheet.spriteCountY;
    GLfloat texCoords[] = {
        u, v + spriteHeight,
        u + spriteWidth, v,
        u, v,
        u + spriteWidth, v,
        u, v + spriteHeight,
        u + spriteWidth, v + spriteHeight
    };
    float aspect = width / height;
    float vertices[] = {
        -0.5f * aspect * TILE_SIZE, -0.5f * TILE_SIZE,
        0.5f * aspect * TILE_SIZE, 0.5f * TILE_SIZE,
        -0.5f * aspect * TILE_SIZE, 0.5f * TILE_SIZE,
        0.5f * aspect * TILE_SIZE, 0.5f * TILE_SIZE,
        -0.5f * aspect * TILE_SIZE, -0.5f * TILE_SIZE,
        0.5f * aspect * TILE_SIZE, -0.5f * TILE_SIZE
    };

    glBindTexture(GL_TEXTURE_2D, sheet.textureID);

    glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(p.positionAttribute);
    glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(p.texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(p.positionAttribute);
    glDisableVertexAttribArray(p.texCoordAttribute);
}

bool Entity::Update(float elapsed, FlareMap* map) {
    if (!isStatic) {
        collidedBottom = collidedTop = dangerCollide = false;
        wallJumpFrames += elapsed;
        if (wallJumpFrames > 0.4f) {
            wallJump = collidedLeft = collidedRight = false;
            wallJumpFrames = 0.0f;
        }

        velX = lerp(velX, 0.0f, elapsed * fricX);
        velY = lerp(velY, 0.0f, elapsed * fricY);

        velX += accX * elapsed;
        velY += gravityY * elapsed;

        x += velX * elapsed;
        y += velY * elapsed;
        return checkTileCollision(map);
    }
    return false; 
}

void Entity::resolveCollisionX(Entity& entity) {
    float x_dist = x - entity.x;
    float x_pen = fabs(x_dist - width - entity.width);
    if (entity.x > x) {
        x -= x_pen + 0.1f;
    }
    else {
        x += x_pen + 0.1f;
    }
}

void Entity::resolveCollisionY(Entity& entity) {
    float y_dist = y - entity.y;
    float y_pen = fabs(y_dist - height - entity.height);
    if (entity.y > y) {
        y -= y_pen + 0.05f;
    }
    else {
        y += y_pen + 0.05f;
    }
}

bool Entity::CollidesWith(Entity& entity) {
    float x_dist = fabs(x - entity.x) - ((width * 0.5f) + (entity.width * 0.5f));
    float y_dist = fabs(y - entity.y) - ((height * 0.5f) + (entity.height * 0.5f));
    return (x_dist <= 0 && y_dist <= 0);
}

bool Entity::checkTileCollision(FlareMap *map) {

    int gridX;
    int gridY;

    //bottom
    worldToTileCoordinates(x, y - 0.5f * height, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
            collidedBottom = true;
            velY = 0.0f;
            accY = 0.0f;
            float penetration = fabs((-TILE_SIZE * gridY) - (y - height / 2));
            y += penetration + (TILE_SIZE * 0.00000000001f);
        }
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            dangerCollide = true;
        }
    }
    else {
        return true;
    }

    //top
    worldToTileCoordinates(x, y + 0.5f * height, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
            collidedTop = true;
            velY = 0.0f;
            accY = 0.0f;
            float penetration = fabs(((-TILE_SIZE * gridY) - TILE_SIZE) - (y + height / 2));
            y -= penetration + (TILE_SIZE * 0.00000000001f);
        }
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            dangerCollide = true;
        }
    }

    //left
    worldToTileCoordinates(x - 0.5f * width, y, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
            if (map->mapData[gridY][gridX] == 6) {
                wallJump = true;
                wallJumpFrames += 0.001f;
            }
            collidedLeft = true;
            velX = 0.0f;
            accX = 0.0f;
            float penetration = fabs(((TILE_SIZE * gridX) + TILE_SIZE) - (x - width / 2));
            x += penetration + (TILE_SIZE * 0.00000000001f);
        }
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            dangerCollide = true;
        }
    }

    //right
    worldToTileCoordinates(x + 0.5f * width, y, &gridX, &gridY);
    if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
        if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
            if (map->mapData[gridY][gridX] == 6) {
                wallJump = true;
                wallJumpFrames += 0.001f;
            }
            collidedRight = true;
            velX = 0.0f;
            accX = 0.0f;
            float penetration = fabs((TILE_SIZE * gridX) - (x + width / 2));
            x -= penetration + (TILE_SIZE * 0.00000000001f);
        }
        if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
            dangerCollide = true;
        }
    }
    return dangerCollide;
}