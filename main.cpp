#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#define GL_GLEXT_PROTOTYPES 1
#include <SDL_opengl.h>
#include <SDL_image.h>

#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ShaderProgram.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <vector>
#include <iostream>
#include <algorithm>
#include <ostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <string>
#include <math.h>

using namespace std;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define TILE_SIZE 0.1f
float lastFrameTicks = 0.0f;
std::vector<int> solidTiles = {0, 1, 2, 16, 17, 18, 19, 32, 33, 34, 35};
std::vector<int> dangerTiles = {100, 101};
std::vector<int> climbTile = {0, 1, 2, 6, 16, 17, 18, 19, 32, 33, 34, 35};


enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL1, STATE_GAME_LEVEL2, STATE_GAME_LEVEL3, STATE_GAME_OVER };
enum EntityType {PLAYER, ANNOYING, WINTOKEN};

SDL_Window* displayWindow;


float lerp(float v0, float v1, float t) {
    return (1.0f - t)*v0 + t * v1;
}

GLuint LoadTexture(const char *filePath) {
    int w, h, comp;
    unsigned char* image = stbi_load(filePath, &w, &h, &comp, STBI_rgb_alpha);
    
    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        
    }
    
    GLuint retTexture;
    glGenTextures(1, &retTexture);
    glBindTexture(GL_TEXTURE_2D, retTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    
    stbi_image_free(image);
    return retTexture;
}

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY) {
    *gridX = (int)(worldX / TILE_SIZE);
    *gridY = (int)(worldY / -TILE_SIZE);
}

struct FlareMapEntity {
    std::string type;
    float x;
    float y;
};

class FlareMap {
public:
    FlareMap() {
        mapData = nullptr;
        mapWidth = -1;
        mapHeight = -1;
    }
    ~FlareMap() {
        for (int i = 0; i < mapHeight; i++) {
            delete mapData[i];
        }
        delete mapData;
    }
    
    void Load(const std::string fileName) {
        std::ifstream infile(fileName);
        if (infile.fail()) {
            assert(false); // unable to open file
        }
        std::string line;
        while (std::getline(infile, line)) {
            if (line == "[header]") {
                if (!ReadHeader(infile)) {
                    assert(false); // invalid file data
                }
            }
            else if (line == "[layer]") {
                ReadLayerData(infile);
            }
            else if (line == "[ObjectsLayer]") {
                ReadEntityData(infile);
            }
        }
    }
    
    int mapWidth;
    int mapHeight;
    unsigned int **mapData;
    std::vector<FlareMapEntity> entities;
    
private:
    
    bool ReadHeader(std::ifstream &stream) {
        std::string line;
        mapWidth = -1;
        mapHeight = -1;
        while (std::getline(stream, line)) {
            if (line == "") { break; }
            std::istringstream sStream(line);
            std::string key, value;
            std::getline(sStream, key, '=');
            std::getline(sStream, value);
            if (key == "width") {
                mapWidth = std::atoi(value.c_str());
            }
            else if (key == "height") {
                mapHeight = std::atoi(value.c_str());
            }
        }
        if (mapWidth == -1 || mapHeight == -1) {
            return false;
        }
        else {
            mapData = new unsigned int*[mapHeight];
            for (int i = 0; i < mapHeight; ++i) {
                mapData[i] = new unsigned int[mapWidth];
            }
            return true;
        }
    }
    
    bool ReadLayerData(std::ifstream &stream) {
        std::string line;
        while (getline(stream, line)) {
            if (line == "") { break; }
            std::istringstream sStream(line);
            std::string key, value;
            std::getline(sStream, key, '=');
            std::getline(sStream, value);
            if (key == "data") {
                for (int y = 0; y < mapHeight; y++) {
                    getline(stream, line);
                    std::istringstream lineStream(line);
                    std::string tile;
                    for (int x = 0; x < mapWidth; x++) {
                        std::getline(lineStream, tile, ',');
                        unsigned int val = atoi(tile.c_str());
                        if (val > 0) {
                            mapData[y][x] = val - 1;
                        }
                        else {
                            mapData[y][x] = -1;
                        }
                    }
                }
            }
        }
        return true;
    }
    
    bool ReadEntityData(std::ifstream &stream) {
        std::string line;
        std::string type;
        while (getline(stream, line)) {
            if (line == "") { break; }
            std::istringstream sStream(line);
            std::string key, value;
            getline(sStream, key, '=');
            getline(sStream, value);
            if (key == "type") {
                type = value;
            }
            else if (key == "location") {
                std::istringstream lineStream(value);
                std::string xPosition, yPosition;
                getline(lineStream, xPosition, ',');
                getline(lineStream, yPosition, ',');
                
                FlareMapEntity newEntity;
                newEntity.type = type;
                newEntity.x = std::atoi(xPosition.c_str()) * TILE_SIZE;
                newEntity.y = std::atoi(yPosition.c_str()) * -TILE_SIZE;
                entities.push_back(newEntity);
            }
        }
        return true;
    }
    
};

struct SpriteSheet {
    unsigned int textureID;
    int spriteCountX;
    int spriteCountY;
    
    SpriteSheet() {}
    SpriteSheet(unsigned int textureID_, int x, int y) {
        textureID = textureID_;
        spriteCountX = x;
        spriteCountY = y;
    }
};

class Entity {
public:
    Entity() {}
    Entity(float x_, float y_, float width_, float height_, bool isStatic_) {
        x = x_;
        y = y_;
        width = width_;
        height = height_;
        isStatic = isStatic_;
    }
    
    float x = 0.0f;
    float y = 0.0f;
    float width;
    float height;
    float velX = 0.0f;
    float velY = 0.0f;
    float accX = 0.0f;
    float accY = 0.0f;
    float fricX = 1.5f;
    float fricY = 0.0f;
    float gravityY = -1.3f;
    
    SpriteSheet sheet;
    
    std::vector<int> sprites;
    int spriteIndex;
    
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedLeft = false;
    bool collidedRight = false;
    bool dangerCollide = false;
    bool isStatic = true;
    
    void Render(ShaderProgram& p) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
        p.SetModelMatrix(modelMatrix);
        
        float u = (float)(((int)sprites[spriteIndex]) % sheet.spriteCountX) / (float)sheet.spriteCountX;
        float v = (float)(((int)sprites[spriteIndex]) / sheet.spriteCountX) / (float)sheet.spriteCountY;
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
    
    bool Update(float elapsed, FlareMap* map) {
        collidedBottom = collidedLeft = collidedRight = collidedTop = false;
        
        velX = lerp(velX, 0.0f, elapsed * fricX);
        velY = lerp(velY, 0.0f, elapsed * fricY);
        
        velX += accX * elapsed;
        velY += gravityY * elapsed;
        
        x += velX * elapsed;
        y += velY * elapsed;
        checkTileCollision(map);
        checkWallCollision(map);
        return checkDangerCollision(map);
    }
    
    void resolveCollisionX(Entity& entity) {
        float x_dist = x - entity.x;
        float x_pen = fabs(x_dist - width - entity.width);
        if (entity.x > x) {
            x -= x_pen + 0.1f;
        }
        else {
            x += x_pen + 0.1f;
        }
    }
    
    void resolveCollisionY(Entity& entity) {
        float y_dist = y - entity.y;
        float y_pen = fabs(y_dist - height - entity.height);
        if (entity.y > y) {
            y -= y_pen + 0.05f;
        }
        else {
            y += y_pen + 0.05f;
        }
    }
    
    bool CollidesWith(Entity& entity) {
        float x_dist = fabs(x - entity.x) - (width + entity.width);
        float y_dist = fabs(y - entity.y) - (height + entity.height);
        return (x_dist <= 0 && y_dist <= 0);
    }
    
    void checkTileCollision(FlareMap *map) {
        
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
        }
        
        //top
        worldToTileCoordinates(x, y + 0.5 * height, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
                collidedTop = true;
                velY = 0.0f;
                accY = 0.0f;
                float penetration = fabs(((-TILE_SIZE * gridY) - TILE_SIZE) - (y + height / 2));
                y -= penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
        
        //left
        worldToTileCoordinates(x - 0.5 * width, y, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
                collidedLeft = true;
                velX = 0.0f;
                accX = 0.0f;
                float penetration = fabs(((TILE_SIZE * gridX) + TILE_SIZE) - (x - width / 2));
                x += penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
        
        //right
        worldToTileCoordinates(x + 0.5 * width, y, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridX]) != solidTiles.end()) {
                collidedRight = true;
                velX = 0.0f;
                accX = 0.0f;
                float penetration = fabs((TILE_SIZE * gridX) - (x + width / 2));
                x -= penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
    }
    
    bool checkDangerCollision(FlareMap *map) {
        
        int gridX;
        int gridY;
        
        //bottom
        worldToTileCoordinates(x, y - 0.5f * height, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
                dangerCollide = true;
                velY = 0.0f;
                accY = 0.0f;
            }
        }
        
        //top
        worldToTileCoordinates(x, y + 0.5 * height, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
                dangerCollide = true;
                velY = 0.0f;
                accY = 0.0f;
            }
        }
        
        //left
        worldToTileCoordinates(x - 0.5 * width, y, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
                dangerCollide = true;
                velX = 0.0f;
                accX = 0.0f;
            }
        }
        
        //right
        worldToTileCoordinates(x + 0.5 * width, y, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
                dangerCollide = true;
                velX = 0.0f;
                accX = 0.0f;
            }
        }
        return dangerCollide;
    }
    
    void checkWallCollision(FlareMap *map) {
        
        int gridX;
        int gridY;
        
        //bottom
        worldToTileCoordinates(x, y - 0.5f * height, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(climbTile.begin(), climbTile.end(), map->mapData[gridY][gridX]) != climbTile.end()) {
                collidedBottom = true;
                velY = 0.0f;
                accY = 0.0f;
                float penetration = fabs((-TILE_SIZE * gridY) - (y - height / 2));
                y += penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
        
        //top
        worldToTileCoordinates(x, y + 0.5 * height, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(climbTile.begin(), climbTile.end(), map->mapData[gridY][gridX]) != climbTile.end()) {
                collidedTop = true;
                velY = 0.0f;
                accY = 0.0f;
                float penetration = fabs(((-TILE_SIZE * gridY) - TILE_SIZE) - (y + height / 2));
                y -= penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
        
        //left
        worldToTileCoordinates(x - 0.5 * width, y, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(climbTile.begin(), climbTile.end(), map->mapData[gridY][gridX]) != climbTile.end()) {
                collidedLeft = true;
                velX = 1.0f;
                accX *= -1.0f;
                velY = 0.5f;
                float penetration = fabs(((TILE_SIZE * gridX) + TILE_SIZE) - (x - width / 2));
                x += penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
        
        //right
        worldToTileCoordinates(x + 0.5 * width, y, &gridX, &gridY);
        if (gridX >= 0 && gridX < map->mapWidth && gridY >= 0 && gridY < map->mapHeight) {
            if (std::find(climbTile.begin(), climbTile.end(), map->mapData[gridY][gridX]) != climbTile.end()) {
                collidedRight = true;
                velX = -1.0f;
                accX *= -1.0f;
                velY = 0.5f;
                float penetration = fabs((TILE_SIZE * gridX) - (x + width / 2));
                x -= penetration + (TILE_SIZE * 0.00000000001f);
            }
        }
    }
    
};

struct GameState {
    SpriteSheet Texture;
    SpriteSheet PlayerSprites;
    GLuint font;
    
    GameMode mode = STATE_MAIN_MENU;
    
    FlareMap level1 = FlareMap();
    FlareMap level2 = FlareMap();
    FlareMap level3 = FlareMap();
    
    //int level = 1;
    
    Entity player;
    Entity annoying;
    Entity wintoken;
    ShaderProgram program;
    
    GameState(ShaderProgram& p) {
        program = p;
    }
    
    void Render() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        switch (mode) {
            case (STATE_MAIN_MENU):
                RenderMenu();
                break;
                
                
            case(STATE_GAME_LEVEL1):
                RenderLevel(level1);
                player.Render(program);
                //annoying.Render(program);
                //wintoken.Render(program);
                break;
                
                
            case(STATE_GAME_LEVEL2):
                RenderLevel(level2);
                player.Render(program);
                break;
                
                
            case(STATE_GAME_LEVEL3):
                RenderLevel(level3);
                player.Render(program);
                break;
                
                
            case (STATE_GAME_OVER):
                DrawText("oof", 0.3f, 0.0f, -1.1f, 0.8f);
                break;
        }
    }
    
    void RenderLevel(FlareMap& map) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        float xOffset = std::min(std::max(-player.x, ((float)map.mapWidth * -TILE_SIZE) + 1.777f), -1.777f);
        float yOffset = std::min(std::max(-player.y, ((float)map.mapHeight * -TILE_SIZE)), 1.0f);
        viewMatrix = glm::translate(viewMatrix, glm::vec3(xOffset, yOffset, 0.0f));
        program.SetModelMatrix(modelMatrix);
        program.SetViewMatrix(viewMatrix);
        DrawTiles(map);
    }
    
    void RenderMenu() {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        program.SetViewMatrix(viewMatrix);
        program.SetModelMatrix(modelMatrix);
        DrawText("OOf", 0.2f, 0.00001f, -0.5f, -0.0f);
        DrawText("PRESS THE SPACEBAR", 0.1f, 0.00001f, -0.4f, -0.5f);
        DrawText("PRESS Q to QUIT", 0.05f, 0.00001f, 1.0f, -0.9f);
    }
    
    void Update(float elapsed) {
        switch (mode) {
            case (STATE_MAIN_MENU):
                break;
                
            case(STATE_GAME_LEVEL1):
                player.Update(elapsed, &level1);
                if (player.Update(elapsed, &level1)) {
                    mode = STATE_MAIN_MENU;
                }
                //                if (player.CollidesWith(wintoken)) {
                //                    mode = STATE_GAME_LEVEL2;
                //                }
                
                break;
            case(STATE_GAME_LEVEL2):
                player.Update(elapsed, &level2);
                if (player.Update(elapsed, &level2)) {
                    mode = STATE_MAIN_MENU;
                }
                //                if (player.CollidesWith(wintoken)) {
                //                    mode = STATE_GAME_LEVEL3;
                //                }
                
                break;
            case(STATE_GAME_LEVEL3):
                player.Update(elapsed, &level3);
                if (player.Update(elapsed, &level3)) {
                    mode = STATE_MAIN_MENU;
                }
                
                break;
                
        }
    }
    
    void ProcessInput(const Uint8 * keys) {
        switch (mode) {
            case (STATE_MAIN_MENU):
                if (keys[SDL_SCANCODE_SPACE]) {
                    mode = STATE_GAME_LEVEL1;
                    SetEntities();
                }
                break;
                
            case (STATE_GAME_LEVEL1):
            case (STATE_GAME_LEVEL2):
            case (STATE_GAME_LEVEL3):
                
                if (keys[SDL_SCANCODE_LEFT]) {
                    player.accX = -1.0f;
                }
                if (keys[SDL_SCANCODE_RIGHT]) {
                    player.accX = 1.0f;
                }
                if (!(keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_RIGHT])) {
                    player.accX = 0.0f;
                }
                if (keys[SDL_SCANCODE_UP]) {
                    player.x = 0.0f;
                    player.y = 0.0f;
                }
                if (keys[SDL_SCANCODE_SPACE]) {
                    if (player.collidedBottom) {
                        player.velY = 1.0f;
                    }
                }
                if (keys[SDL_SCANCODE_Q]) {
                    mode = STATE_MAIN_MENU;
                    player.x = 0.0f;
                    player.y = 0.0f;
                    player.velX = 0.0f;
                    player.velY = 0.0f;
                }
                break;
                
            case (STATE_GAME_OVER):
                if (keys[SDL_SCANCODE_SPACE]) {
                    mode = STATE_MAIN_MENU;
                    SetEntities();
                }
                break;
        }
        
    }
    
    void DrawTiles(FlareMap& map) {
        std::vector<float> vertexData;
        std::vector<float> texCoordData;
        for (int y = 0; y < map.mapHeight; y++) {
            for (int x = 0; x < map.mapWidth; x++) {
                if (map.mapData[y][x] != -1) {
                    float u = (float)(((int)map.mapData[y][x]) % Texture.spriteCountX) / (float)Texture.spriteCountX;
                    float v = (float)(((int)map.mapData[y][x]) / Texture.spriteCountX) / (float)Texture.spriteCountY;
                    float spriteWidth = 1.0f / (float)Texture.spriteCountX;
                    float spriteHeight = 1.0f / (float)Texture.spriteCountY;
                    vertexData.insert(vertexData.end(), {
                        TILE_SIZE * x, -TILE_SIZE * y,
                        TILE_SIZE * x, (-TILE_SIZE * y) - TILE_SIZE,
                        (TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
                        TILE_SIZE * x, -TILE_SIZE * y,
                        (TILE_SIZE * x) + TILE_SIZE, (-TILE_SIZE * y) - TILE_SIZE,
                        (TILE_SIZE * x) + TILE_SIZE, -TILE_SIZE * y
                    });
                    texCoordData.insert(texCoordData.end(), {
                        u, v,
                        u, v + (spriteHeight),
                        u + spriteWidth, v + (spriteHeight),
                        u, v,
                        u + spriteWidth, v + (spriteHeight),
                        u + spriteWidth, v
                    });
                    
                }
            }
        }
        
        glUseProgram(program.programID);
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glBindTexture(GL_TEXTURE_2D, Texture.textureID);
        glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    
    void DrawText(std::string text, float size, float spacing, float posx, float posy) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(posx, posy, 0.0f));
        program.SetModelMatrix(modelMatrix);
        float character_size = 1.0 / 16.0f;
        vector<float> vertexData;
        vector<float> texCoordData;
        for (unsigned int i = 0; i < text.size(); i++) {
            int spriteIndex = (int)text[i];
            float texture_x = (float)(spriteIndex % 16) / 16.0f;
            float texture_y = (float)(spriteIndex / 16) / 16.0f;
            vertexData.insert(vertexData.end(), {
                ((size + spacing) * i) + (-0.5f * size), 0.5f * size,
                ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
                ((size + spacing) * i) + (0.5f * size), 0.5f * size,
                ((size + spacing) * i) + (0.5f * size), -0.5f * size,
                ((size + spacing) * i) + (0.5f * size), 0.5f * size,
                ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
            });
            texCoordData.insert(texCoordData.end(), {
                texture_x, texture_y,
                texture_x, texture_y + character_size,
                texture_x + character_size, texture_y,
                texture_x + character_size, texture_y + character_size,
                texture_x + character_size, texture_y,
                texture_x, texture_y + character_size,
            });
        }
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        glBindTexture(GL_TEXTURE_2D, font);
        
        glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
        
    }
    
    void SetEntities() {
        switch (mode) {
            case (STATE_GAME_LEVEL1):
                player = Entity(
                                level1.entities[0].x + TILE_SIZE, level1.entities[0].y + (TILE_SIZE * 2),
                                TILE_SIZE, TILE_SIZE, false
                                );
                player.sheet = PlayerSprites;
                player.spriteIndex = 0;
                player.sprites = { 0 };
                
                annoying = Entity(
                                  level1.entities[0].x + (TILE_SIZE * 4), level1.entities[0].y + (TILE_SIZE * 2),
                                  TILE_SIZE, TILE_SIZE, false
                                  );
                annoying.sheet = Texture;
                annoying.spriteIndex = 76;
                annoying.spriteIndex = { 76 };
                
                wintoken = Entity(
                                  level1.entities[0].x + (TILE_SIZE * 8), level1.entities[0].y + (TILE_SIZE * 2),
                                  TILE_SIZE, TILE_SIZE, false
                                  );
                wintoken.sheet = Texture;
                wintoken.spriteIndex = 48;
                wintoken.spriteIndex = { 48 };
                
                
                break;
                
            case (STATE_GAME_LEVEL2):
                player = Entity(
                                level2.entities[0].x, level2.entities[0].y + (TILE_SIZE * 2),
                                TILE_SIZE, TILE_SIZE, false
                                );
                player.sheet = PlayerSprites;
                player.spriteIndex = 0;
                player.sprites = { 0 };
                break;
                
            case (STATE_GAME_LEVEL3):
                player = Entity(
                                level3.entities[0].x, level3.entities[0].y + (TILE_SIZE * 2),
                                TILE_SIZE, TILE_SIZE, false
                                );
                player.sheet = PlayerSprites;
                player.spriteIndex = 0;
                player.sprites = { 0 };
                break;
                
            case (STATE_MAIN_MENU):
            case (STATE_GAME_OVER):
                break;
                //maybe move player entity off screen
        }
    }
    
    void Load() {
        font = LoadTexture(RESOURCE_FOLDER"font1.png");
        Texture = SpriteSheet(LoadTexture(RESOURCE_FOLDER"arne_sprites.PNG"), 16, 8);
        PlayerSprites = SpriteSheet(LoadTexture(RESOURCE_FOLDER"yooyoo.PNG"), 6, 4);
        level1.Load(RESOURCE_FOLDER"level1.txt");
        level2.Load(RESOURCE_FOLDER"level2.txt");
        level3.Load(RESOURCE_FOLDER"level3.txt");
    }
    
};

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("My Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    ShaderProgram program;
    program.Load(RESOURCE_FOLDER"vertex_textured.glsl", RESOURCE_FOLDER"fragment_textured.glsl");
    
    glClearColor(0.039f, 0.596f, 0.674f, 1.0f);
    glm::mat4 projectionMatrix = glm::mat4(1.0f);
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);
    program.SetProjectionMatrix(projectionMatrix);
    glUseProgram(program.programID);
    
    // +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    GameState test = GameState(program);
    test.Load();
    
    SDL_Event event;
    bool done = false;
    while (!done) {
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
            else if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.scancode == SDL_SCANCODE_T) {
                    //move to next level mode
                    if (test.mode == STATE_GAME_LEVEL1) {
                        test.mode = STATE_GAME_LEVEL2;
                        test.SetEntities();
                        
                    }
                    else if (test.mode == STATE_GAME_LEVEL2) {
                        test.mode = STATE_GAME_LEVEL3;
                        test.SetEntities();
                        
                    }
                    else if (test.mode == STATE_GAME_LEVEL3) {
                        test.mode = STATE_MAIN_MENU;
                    }
                }
            }
        }
        test.ProcessInput(keys);
        
        
        test.Render();
        test.Update(elapsed);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}





