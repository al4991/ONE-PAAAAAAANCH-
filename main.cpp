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
#include <SDL_mixer.h>

using namespace std;

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define TILE_SIZE 0.1f
bool done = false;
float lastFrameTicks = 0.0f;
float accumulator = 0.0f;
float animationElapsed = 0.0f;
float framesPerSecond = 15.0f;
std::vector<int> solidTiles = { 0, 1, 2, 5, 6, 16, 17, 18, 19, 32, 33, 34, 35, 100, 101, };
std::vector<int> dangerTiles = { 100, 101 };
std::vector<int> climbTile = { 6 };




enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL1, STATE_GAME_LEVEL2, STATE_GAME_LEVEL3, STATE_GAME_OVER, STATE_WIN };

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
    
    
    void justDraw(ShaderProgram& program, int index) {
        glBindTexture(GL_TEXTURE_2D, textureID);
        float u = (float)(((int)index) % spriteCountX) / (float)spriteCountX;
        float v = (float)(((int)index) / spriteCountX) / (float)spriteCountY;
        float width = 1.0 / (float)spriteCountX;
        float height = 1.0 / (float)spriteCountY;
        GLfloat texCoords[] = {
            u, v + height,
            u + width, v,
            u, v,
            u + width, v,
            u, v + height,
            u + width, v + height
        };
        float vertices[] = {
            -0.5f * TILE_SIZE  , -0.5f * TILE_SIZE,
            0.5f * TILE_SIZE , 0.5f * TILE_SIZE  ,
            -0.5f * TILE_SIZE  , 0.5f * TILE_SIZE  ,
            0.5f * TILE_SIZE  , 0.5f * TILE_SIZE ,
            -0.5f * TILE_SIZE , -0.5f * TILE_SIZE  ,
            0.5f * TILE_SIZE  , -0.5f * TILE_SIZE
        };
        
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(program.positionAttribute);
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
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
    float gravityY = -2.2f;
    
    SpriteSheet sheet;
    
    std::vector<int> sprites;
    std::vector<int> forwardSprites;
    std::vector<int> backwardSprites;
    int spriteIndex;
    int spriteSet = 0;
    float wallJumpFrames = 0.0f;
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedLeft = false;
    bool collidedRight = false;
    bool dangerCollide = false;
    bool wallJump = false;
    bool isStatic = true;
    
    void Render(ShaderProgram& p) {
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
    
    bool Update(float elapsed, FlareMap* map) {
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
    
    //void resolveCollisionX(Entity& entity) {
    //  float x_dist = x - entity.x;
    //  float x_pen = fabs(x_dist - width - entity.width);
    //  if (entity.x > x) {
    //      x -= x_pen + 0.1f;
    //  }
    //  else {
    //      x += x_pen + 0.1f;
    //  }
    //}
    
    //void resolveCollisionY(Entity& entity) {
    //  float y_dist = y - entity.y;
    //  float y_pen = fabs(y_dist - height - entity.height);
    //  if (entity.y > y) {
    //      y -= y_pen + 0.05f;
    //  }
    //  else {
    //      y += y_pen + 0.05f;
    //  }
    //}
    
    bool CollidesWith(Entity& entity) {
        float x_dist = fabs(x - entity.x) - ((width * 0.5f) + (entity.width * 0.5f));
        float y_dist = fabs(y - entity.y) - ((height * 0.5f) + (entity.height * 0.5f));
        return (x_dist <= 0 && y_dist <= 0);
    }
    
    bool checkTileCollision(FlareMap *map) {
        
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
            if (std::find(dangerTiles.begin(), dangerTiles.end(), map->mapData[gridY][gridX]) != dangerTiles.end()) {
                dangerCollide = true;
            }
        }
        
        //left
        worldToTileCoordinates(x - 0.5 * width, y, &gridX, &gridY);
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
        worldToTileCoordinates(x + 0.5 * width, y, &gridX, &gridY);
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
};

struct Hitbox {
    Entity body;
    float timeAlive;
    bool direction;
    
    bool checkFulfill(FlareMap* map) {
        int gridX;
        int gridY;
        
        //top
        worldToTileCoordinates(body.x, body.y + 0.5 * body.height, &gridX, &gridY);
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
        return false;
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
    
    Entity player;
    vector<Entity> annoying;
    Entity victory;
    ShaderProgram program;
    Hitbox* hitbox = NULL;
    Mix_Music *background;
    Mix_Chunk *door;
    
    
    GameState(ShaderProgram& p) {
        program = p;
    }
    
    ~GameState() {
        Mix_FreeMusic(background);
        Mix_FreeChunk(door);
    }
    
    void loadMusic() {
        Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
        background = Mix_LoadMUS(RESOURCE_FOLDER"background.mp3");
        door = Mix_LoadWAV(RESOURCE_FOLDER"door.wav");
    }
    
    void backgroundMusic() {
        Mix_PlayMusic(background, -1);
        Mix_VolumeMusic(30);
    }
    
    void playSound() {
        Mix_PlayChannel(0, door, .5);
        Mix_VolumeMusic(25);
    }
    
    
    void Render() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        switch (mode) {
                
            case (STATE_MAIN_MENU):
                RenderMenu();
                break;
            case (STATE_GAME_OVER):
                RenderGameOver();
                break;
                
            case(STATE_WIN):
                RenderWin();
                break;
                
            case(STATE_GAME_LEVEL1):
                if (hitbox) {
                    hitbox->body.Render(program);
                }
                RenderLevel(level1);
                
                player.Render(program);
                for (Entity i : annoying) {
                    i.Render(program);
                }
                victory.Render(program);
                break;
                
                
            case(STATE_GAME_LEVEL2):
                if (hitbox) {
                    hitbox->body.Render(program);
                }
                RenderLevel(level2);
                
                player.Render(program);
                for (Entity i : annoying) {
                    i.Render(program);
                }
                victory.Render(program);
                
                break;
                
                
            case(STATE_GAME_LEVEL3):
                if (hitbox) {
                    hitbox->body.Render(program);
                }
                RenderLevel(level3);
                player.Render(program);
                for (Entity i : annoying) {
                    i.Render(program);
                }
                victory.Render(program);
                break;
        }
    }
    
    void RenderLevel(FlareMap& map) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        float xOffset = std::min(std::max(-player.x, ((float)map.mapWidth * -TILE_SIZE) + 1.777f), -1.777f);
        float yOffset = std::min(std::max(-player.y, ((float)map.mapHeight * TILE_SIZE)), 2.0f);
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
        DrawText("Oof", 0.2f, 0.0f, -0.55f, 0.475f);
        DrawText("the game", 0.1f, 0.0f, 0.0f, 0.45f);
        DrawText("PRESS SPACE TO START", 0.075f, 0.0f, -0.7125f, -0.5f);
        DrawText("PRESS Q to QUIT", 0.05f, 0.00001f, 1.0f, -0.9f);
        DrawText("By Mithila", 0.05f, 0.0f, -1.7f, -0.825f);
        DrawText("    and", 0.05f, 0.0f, -1.7f, -0.875f);
        DrawText("  Andrew", 0.05f, 0.0f, -1.7f, -0.925f);
        
    }
    
    void RenderGameOver() {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        program.SetViewMatrix(viewMatrix);
        program.SetModelMatrix(modelMatrix);
        DrawText("Ya lost, it's over", .15f, 0.0f, -1.2f, -0.0f);
        DrawText("Press Space to", 0.075f, 0.0f, -0.6125f, -0.5f);
        DrawText("go back to Menu", 0.075f, 0.0f, -0.6125f, -0.575f);
    }
    
    void RenderWin() {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        program.SetViewMatrix(viewMatrix);
        program.SetModelMatrix(modelMatrix);
        DrawText("You win", 0.15f, 0.0f, -0.45f, 0.0f);
    }
    
    void Update(float elapsed) {
        switch (mode) {
            case (STATE_MAIN_MENU):
            case (STATE_GAME_OVER):
            case (STATE_WIN):
                break;
                
            case(STATE_GAME_LEVEL1):
                if (player.CollidesWith(victory)) {
                    mode = STATE_GAME_LEVEL2;
                    SetEntities();
                    playSound();
                }
                
                if (hitbox) {
                    hitbox->timeAlive += elapsed;
                    if (hitbox->timeAlive > 1.0f) {
                        hitbox = NULL;
                    }
                    else {
                        hitbox->body.x = player.x;
                        hitbox->body.y = hitbox->direction ?
                        player.y + (player.height*0.5f) + (TILE_SIZE * 0.5f) :
                        player.y - (player.height*0.5f) - (TILE_SIZE * 0.5f);
                        
                        if (hitbox->checkFulfill(&level1)) {
                            player.velY = hitbox->direction ? -1.0f : 1.0f;
                            hitbox = NULL;
                        }
                    }
                }
                for (Entity& i : annoying) {
                    if (i.isStatic) {
                        if (fabs(i.x - player.x) < TILE_SIZE) {
                            i.isStatic = false;
                        }
                    }
                    else {
                        float diff = i.x - player.x;
                        i.velX = diff < 0 ? 0.35f : -0.35f;
                        i.Update(elapsed, &level1);
                    }
                    if (player.CollidesWith(i)) {
                        mode = STATE_GAME_OVER;
                    }
                    if (hitbox) {
                        if (i.CollidesWith(hitbox->body)) {
                            hitbox = NULL;
                            i.y -= 1000.0f;
                        }
                    }
                }
                
                if (player.Update(elapsed, &level1)) {
                    mode = STATE_GAME_OVER;
                }
                
                break;
                
            case(STATE_GAME_LEVEL2):
                if (player.CollidesWith(victory)) {
                    mode = STATE_GAME_LEVEL3;
                    SetEntities();
                    playSound();
                }
                if (hitbox) {
                    hitbox->timeAlive += elapsed;
                    if (hitbox->timeAlive > 1.0f) {
                        hitbox = NULL;
                    }
                    else {
                        hitbox->body.x = player.x;
                        hitbox->body.y = hitbox->direction ?
                        player.y + (player.height*0.5f) + (TILE_SIZE * 0.5f) :
                        player.y - (player.height*0.5f) - (TILE_SIZE * 0.5f);
                        
                        if (hitbox->checkFulfill(&level2)) {
                            player.velY = hitbox->direction ? -1.0f : 1.0f;
                            hitbox = NULL;
                        }
                        /*for (Entity& i : annoying) {
                         if (hitbox->body.CollidesWith(i)) {
                         i.y -= 3000.0f;
                         hitbox = NULL;
                         break;
                         }
                         }*/
                    }
                    
                }
                
                if (player.Update(elapsed, &level2)) {
                    mode = STATE_GAME_OVER;
                }
                break;
                
            case(STATE_GAME_LEVEL3):
                if (player.CollidesWith(victory)) {
                    mode = STATE_WIN;
                    playSound();
                }
                if (hitbox) {
                    hitbox->timeAlive += elapsed;
                    if (hitbox->timeAlive > 1.0f) {
                        hitbox = NULL;
                    }
                    else {
                        hitbox->body.x = player.x;
                        hitbox->body.y = hitbox->direction ?
                        player.y + (player.height*0.5f) + (TILE_SIZE * 0.5f) :
                        player.y - (player.height*0.5f) - (TILE_SIZE * 0.5f);
                        
                        if (hitbox->checkFulfill(&level3)) {
                            player.velY = hitbox->direction ? -1.0f : 1.0f;
                            hitbox = NULL;
                        }
                    }
                }
                
                for (Entity& i : annoying) {
                    if (i.isStatic) {
                        if (fabs(i.x - player.x) < TILE_SIZE) {
                            i.isStatic = false;
                        }
                    }
                    else {
                        float diff = i.x - player.x;
                        i.velX = diff < 0 ? 0.35f : -0.35f;
                        i.Update(elapsed, &level3);
                    }
                    if (player.CollidesWith(i)) {
                        mode = STATE_GAME_OVER;
                    }
                    if (hitbox) {
                        if (i.CollidesWith(hitbox->body)) {
                            hitbox = NULL;
                            i.y -= 1000.0f;
                        }
                    }
                }
                
                if (player.Update(elapsed, &level3)) {
                    mode = STATE_GAME_OVER;
                }
                break;
        }
    }
    
    void ProcessInput(const Uint8 * keys) {
        switch (mode) {
            case (STATE_MAIN_MENU):
                //if (keys[SDL_SCANCODE_SPACE]) {
                //  mode = STATE_GAME_LEVEL1;
                //  SetEntities();
                //}
                //break;
            case (STATE_GAME_OVER):
                //if (keys[SDL_SCANCODE_SPACE]) {
                //  mode = STATE_MAIN_MENU;
                //  SetEntities();
                //}
                //break;
            case (STATE_WIN):
                /*  if (keys[SDL_SCANCODE_SPACE]) {
                 mode = STATE_MAIN_MENU;
                 SetEntities();
                 }*/
                break;
                
            case (STATE_GAME_LEVEL1):
            case (STATE_GAME_LEVEL2):
            case (STATE_GAME_LEVEL3):
                
                if (keys[SDL_SCANCODE_LEFT]) {
                    player.accX = -1.05f;
                    player.spriteSet = 1;
                }
                if (keys[SDL_SCANCODE_RIGHT]) {
                    player.accX = 1.05f;
                    player.spriteSet = 2;
                    
                }
                if (!(keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_RIGHT])) {
                    player.accX = 0.0f;
                    player.spriteSet = 0;
                    
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
        }
        
    }
    
    void ProcessEvent(SDL_Event event) {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
            done = true;
        }
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.scancode == SDL_SCANCODE_SPACE) {
                if (mode == STATE_MAIN_MENU) {
                    mode = STATE_GAME_LEVEL1;
                    SetEntities();
                }
                else if (mode == STATE_GAME_OVER) {
                    mode = STATE_MAIN_MENU;
                    SetEntities();
                }
                else if (mode == STATE_WIN) {
                    mode = STATE_MAIN_MENU;
                    SetEntities();
                }
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_T) {
                //move to next level mode
                if (mode == STATE_GAME_LEVEL1) {
                    mode = STATE_GAME_LEVEL2;
                    SetEntities();
                    
                }
                else if (mode == STATE_GAME_LEVEL2) {
                    mode = STATE_GAME_LEVEL3;
                    SetEntities();
                    
                }
                else if (mode == STATE_GAME_LEVEL3) {
                    mode = STATE_MAIN_MENU;
                }
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_A) {
                if (player.wallJump && player.collidedLeft) {
                    player.wallJump = player.collidedLeft = false;
                    player.wallJumpFrames = 0.0f;
                    player.velX = TILE_SIZE * 20;
                    player.accX = 0.5f;
                    player.velY = TILE_SIZE * 8;
                }
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_D) {
                if (player.wallJump && player.collidedRight) {
                    player.wallJump = player.collidedRight = false;
                    
                    player.wallJumpFrames = 0.0f;
                    player.velX = -TILE_SIZE * 20;
                    player.accX = -0.5f;
                    player.velY = TILE_SIZE * 8;
                }
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_W) {
                if (hitbox == NULL) {
                    hitbox = new Hitbox();
                    hitbox->timeAlive = 0.0f;
                    hitbox->direction = true;
                    hitbox->body = Entity(player.x, player.y + (player.height*0.5f) + (TILE_SIZE * 0.5f), TILE_SIZE, TILE_SIZE * 0.5f, true);
                    hitbox->body.sheet = Texture;
                    hitbox->body.spriteIndex = 0;
                    hitbox->body.sprites = { 39 };
                }
            }
            if (event.key.keysym.scancode == SDL_SCANCODE_S) {
                if (hitbox == NULL) {
                    hitbox = new Hitbox();
                    hitbox->timeAlive = 0.0f;
                    hitbox->direction = false;
                    hitbox->body = Entity(player.x, player.y - (player.height*0.5f) - (TILE_SIZE * 0.5f), TILE_SIZE, TILE_SIZE * 0.5f, true);
                    hitbox->body.sheet = Texture;
                    hitbox->body.spriteIndex = 0;
                    hitbox->body.sprites = { 38 };
                    
                }
            }
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
    
    Entity placeEnemy(float x, float y) {
        Entity a = Entity(
                          x, y,
                          TILE_SIZE, TILE_SIZE, false
                          );
        a.sheet = Texture;
        a.spriteIndex = 0;
        a.sprites = { 76, 77 };
        a.isStatic = true;
        return a;
    }
    
    void SetEntities() {
        hitbox = NULL;
        annoying = {};
        switch (mode) {
            case (STATE_GAME_LEVEL1):
                player = Entity(
                                level1.entities[0].x, level1.entities[0].y,
                                TILE_SIZE, TILE_SIZE, false
                                );
                player.sheet = PlayerSprites;
                player.spriteIndex = 0;
                player.sprites = { 0, 1, 2 };
                player.forwardSprites = { 6, 7, 8 };
                player.backwardSprites = { 12, 13, 14 };
                
                victory = Entity(
                                 level1.entities[1].x, level1.entities[1].y,
                                 TILE_SIZE, TILE_SIZE, false
                                 );
                victory.sheet = Texture;
                victory.spriteIndex = 0;
                victory.sprites = { 48,48,48,48, 49,49,49,49, 50,50,50,50 };
                
                for (FlareMapEntity i : level1.entities) {
                    if (i.type == "Annoying") {
                        annoying.push_back(placeEnemy(i.x, i.y));
                    }
                }
                break;
                
            case (STATE_GAME_LEVEL2):
                player = Entity(
                                level2.entities[0].x, level2.entities[0].y,
                                TILE_SIZE, TILE_SIZE, false
                                );
                player.sheet = PlayerSprites;
                player.spriteIndex = 0;
                player.sprites = { 0, 1, 2 };
                player.forwardSprites = { 6, 7, 8 };
                player.backwardSprites = { 12, 13, 14 };
                
                victory = Entity(
                                 level2.entities[1].x, level2.entities[1].y,
                                 TILE_SIZE, TILE_SIZE, false
                                 );
                victory.sheet = Texture;
                victory.spriteIndex = 0;
                victory.sprites = { 48,48,48,48, 49,49,49,49, 50,50,50,50 };
                
                break;
                
            case (STATE_GAME_LEVEL3):
                player = Entity(
                                level3.entities[0].x, level3.entities[0].y,
                                TILE_SIZE, TILE_SIZE, false
                                );
                player.sheet = PlayerSprites;
                player.spriteIndex = 0;
                player.sprites = { 0, 1, 2 };
                player.forwardSprites = { 6, 7, 8 };
                player.backwardSprites = { 12, 13, 14 };
                victory = Entity(
                                 level3.entities[1].x, level3.entities[1].y,
                                 TILE_SIZE, TILE_SIZE, false
                                 );
                victory.sheet = Texture;
                victory.spriteIndex = 0;
                victory.sprites = { 48,48,48,48, 49,49,49,49, 50,50,50,50 };
                
                for (FlareMapEntity i : level3.entities) {
                    if (i.type == "Annoying") {
                        annoying.push_back(placeEnemy(i.x, i.y));
                    }
                }
                break;
                
            case (STATE_MAIN_MENU):
            case (STATE_GAME_OVER):
            case (STATE_WIN):
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
        loadMusic();
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
    GameState game = GameState(program);
    game.Load();
    game.backgroundMusic();
    
    SDL_Event event;
    while (!done) {
        
        glClear(GL_COLOR_BUFFER_BIT);
        
        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;
        
        animationElapsed += elapsed;
        if (animationElapsed > 1.0 / framesPerSecond) {
            game.player.spriteIndex++;
            game.victory.spriteIndex++;
            animationElapsed = 0.0f;
            if (game.player.spriteIndex > 2) {
                game.player.spriteIndex = 0;
            }
            if (game.victory.spriteIndex > 11) {
                game.victory.spriteIndex = 0;
            }
        }
        
        
        
        while (SDL_PollEvent(&event)) {
            game.ProcessEvent(event);
        }
        
        game.ProcessInput(keys);
        
        game.Render();
        game.Update(elapsed);
        
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
