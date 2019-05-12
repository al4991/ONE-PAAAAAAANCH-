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
std::vector<int> solidTiles = { 0, 1, 2, 6, 16, 17, 18, 19, 32, 33, 34, 35, 100, 101};

SDL_Window* displayWindow;


float lerp(float v0, float v1, float t) {
    return (1.0 - t)*v0 + t * v1;
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
                
                // In the slides, x and y are multiplied by tilesize
            }
        }
        return true;
    }
    
};

void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing, float posx, float posy) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(posx, posy, 0.0f));
    program.SetModelMatrix(modelMatrix);
    float character_size = 1.0/16.0f;
    vector<float> vertexData;
    vector<float> texCoordData;
    for(int i=0; i < text.size(); i++) {
        int spriteIndex = (int)text[i];
        float texture_x = (float)(spriteIndex % 16) / 16.0f;
        float texture_y = (float)(spriteIndex / 16) / 16.0f;
        vertexData.insert(vertexData.end(), {
            ((size+spacing) * i) + (-0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (0.5f * size), -0.5f * size,
            ((size+spacing) * i) + (0.5f * size), 0.5f * size,
            ((size+spacing) * i) + (-0.5f * size), -0.5f * size,
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
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    
    glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
    
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);
    
}

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
    float fricX = 0.0f;
    float fricY = 0.0f;
    float gravityY = -1.3f;
    
    SpriteSheet sheet;
    
    std::vector<int> sprites;
    int spriteIndex;
    
    bool collidedTop = false;
    bool collidedBottom = false;
    bool collidedLeft = false;
    bool collidedRight = false;
    bool isStatic = true;
    
    void Render(ShaderProgram& p) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(x, y, 0.0f));
        p.SetModelMatrix(modelMatrix);
        
        float u = (float)(((int)sprites[spriteIndex]) % sheet.spriteCountX) / (float)sheet.spriteCountX;
        float v = (float)(((int)sprites[spriteIndex]) / sheet.spriteCountX) / (float)sheet.spriteCountY;
        float spriteWidth = 1.0 / (float)sheet.spriteCountX;
        float spriteHeight = 1.0 / (float)sheet.spriteCountY;
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
        
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBindTexture(GL_TEXTURE_2D, sheet.textureID);
        
        glVertexAttribPointer(p.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(p.positionAttribute);
        glVertexAttribPointer(p.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(p.texCoordAttribute);
        
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glDisableVertexAttribArray(p.positionAttribute);
        glDisableVertexAttribArray(p.texCoordAttribute);
    }
    
    void Update(float elapsed, FlareMap* map) {
        if (!isStatic) {
            velX = lerp(velX, 0.0f, elapsed * fricX);
            velY = lerp(velY, 0.0f, elapsed * fricY);
            
            velX += accX * elapsed;
            velY += gravityY * elapsed;
            
            x += velX * elapsed;
            y += velY * elapsed;
            checkTileCollision(map);
        }
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
        int gridBottomY;
        int gridLeftX;
        int gridRightX;
        int gridTopY;
        
        worldToTileCoordinates(x, y, &gridX, &gridY);
        worldToTileCoordinates(x - (width/2), y - (height / 2), &gridLeftX, &gridBottomY);
        worldToTileCoordinates(x + (width / 2), y + (height / 2), &gridRightX, &gridTopY);
        
        if (gridX >= 0 && gridX < map->mapWidth &&
            gridY >= 0 && gridY < map->mapHeight &&
            gridLeftX >= 0 && gridLeftX < map->mapWidth &&
            gridTopY >= 0 && gridTopY < map->mapHeight &&
            gridRightX >= 0 && gridRightX < map->mapWidth &&
            gridBottomY >= 0 && gridBottomY < map->mapHeight) {
            
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridBottomY][gridX]) != solidTiles.end()) {
                collidedBottom = true;
                float pen = fabs((-TILE_SIZE * gridBottomY) - (y - (height / 2)));
                y += pen;
                velX = 0.0f;
                velY = 0.0f;
                accX = 0.0f;
                accY = 0.0f;
            }
            else {
                collidedBottom = false;
            }
            
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridTopY][gridX]) != solidTiles.end()) {
                collidedTop = true;
                float pen = fabs(((-TILE_SIZE * gridTopY) - TILE_SIZE)  - (y + (height / 2)));
                y -= pen;
                velX = 0.0f;
                velY = 0.0f;
                accX = 0.0f;
                accY = 0.0f;
            }
            else {
                collidedTop = false;
            }
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridLeftX]) != solidTiles.end()) {
                collidedLeft = true;
                float pen = fabs(((TILE_SIZE * gridX)) - (x - (width / 2)));
                x += pen;
                velX = 0.0f;
                accX = 0.0f;
            }
            else {
                collidedLeft = false;
            }
            if (std::find(solidTiles.begin(), solidTiles.end(), map->mapData[gridY][gridRightX]) != solidTiles.end()) {
                collidedRight = true;
                float pen = fabs((TILE_SIZE * gridX + TILE_SIZE) - (x + (width / 2)));
                x -= pen;
                velX = 0.0f;
                accX = 0.0f;
            }
            else {
                collidedRight = false;
            }
        }
    }
};



struct GameState {
    SpriteSheet Texture;
    SpriteSheet FontTexture;
    FlareMap map = FlareMap();
    Entity player = Entity();
    ShaderProgram program;
    
    void Render(ShaderProgram& p) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        float xOffset = std::min(std::max(-player.x, ((float)map.mapWidth * -TILE_SIZE) + 1.777f), -1.777f);
        float yOffset = std::min(std::max(-player.y, ((float)map.mapHeight * -TILE_SIZE) + 4.0f ), 2.0f);
        viewMatrix = glm::translate(viewMatrix, glm::vec3(xOffset, yOffset, 0.0f));
        p.SetModelMatrix(modelMatrix);
        p.SetViewMatrix(viewMatrix);
        
        DrawTiles();
        player.Render(p);
    }

    void Update(ShaderProgram& p, float elapsed, GLuint fontTexture) {
        player.Update(elapsed, &map);
        DrawText(program, fontTexture, "WOW", 0.04f, 0.00001f, player.x + 0.00001f, player.y);
        
    }
    
    void ProcessInput(const Uint8 * keys) {
        // if (keys[SDL_SCANCODE_LEFT]) {
        //     player.velX = -0.5f;
        //     player.velY = 0.0f;
            
        // } if (keys[SDL_SCANCODE_RIGHT]) {
        //     player.velX = 0.5f;
        //     player.velY = 0.0f;
            
        // } if (keys[SDL_SCANCODE_UP]) {
        //     player.velY = 0.5f;
        //     player.velX = 0.0f;
        //     if (player.velY > 2.0f) {
        //         player.velY = 1.25f;
        //     }
        // } if (keys[SDL_SCANCODE_SPACE]) {
        //     if (player.collidedBottom) {
        //         player.velY = 0.5f;
        //         player.collidedBottom = false;
        //     }
        // }
        if (keys[SDL_SCANCODE_LEFT]) {
            test.player.accX = -0.5f; 
        } if (keys[SDL_SCANCODE_RIGHT]) {
            test.player.accX = 0.5f;
        } if (keys[SDL_SCANCODE_UP]) {
            test.player.y = .01f;
        } if (keys[SDL_SCANCODE_SPACE]) {
            if (test.player.collidedBottom) {
                test.player.velY = 1.0f; 
                test.player.collidedBottom = false; 
            }
        }
    }
    
    void DrawTiles() {
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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
        glEnableVertexAttribArray(program.positionAttribute);
        
        glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
        glEnableVertexAttribArray(program.texCoordAttribute);
        
        glBindTexture(GL_TEXTURE_2D, Texture.textureID);
        glDrawArrays(GL_TRIANGLES, 0, vertexData.size() / 2);
        
        glDisableVertexAttribArray(program.positionAttribute);
        glDisableVertexAttribArray(program.texCoordAttribute);
    }
    
};


class MainMenu {
public:
    MainMenu(){}
    void Render(ShaderProgram &program, GLuint fontTexture) {
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        program.SetViewMatrix(viewMatrix);
        DrawText(program, fontTexture, "WELCOME!!", 0.3f, 0.0f, -1.1f, 0.8f);
        DrawText(program, fontTexture, "THIS IS A FUN GAME!", 0.2f, 0.00001f, -1.7f, -0.0f);
        DrawText(program, fontTexture, "PRESS THE SPACEBAR", 0.2f, 0.00001f, -1.75f, -0.5f);
        DrawText(program, fontTexture, "PRESS Q to QUIT", 0.05f, 0.00001f, 1.0f, -0.9f);
    }
    
    void Update(ShaderProgram &program, GLuint fontTexture) {
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        program.SetViewMatrix(viewMatrix);
        DrawText(program, fontTexture, "THIS IS A FUN GAME!", 0.2f, 0.00001f, -1.7f, -0.0f);
        DrawText(program, fontTexture, "PRESS THE SPACEBAR", 0.2f, 0.00001f, -1.72f, -0.5f);
        DrawText(program, fontTexture, "PRESS Q to QUIT", 0.05f, 0.00001f, 1.0f, -0.9f);
    }
};


enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL, STATE_GAME_OVER};
GameMode mode;
MainMenu mainMenu;

void Render(ShaderProgram &program, GLuint fontTexture, float elapsed, GameState& gameState) {
    switch(mode) {
        case STATE_MAIN_MENU:
            mainMenu.Render(program, fontTexture);
            break;
        case STATE_GAME_LEVEL:
            gameState.Render(program);
            break;
        case STATE_GAME_OVER:
            mainMenu.Render(program, fontTexture);
            break;
    }
}

void Update(ShaderProgram program, float elapsed, GLuint fontTexture, GameState& gameState) {
    switch(mode) {
        case STATE_GAME_LEVEL:
            gameState.Update(program, elapsed, fontTexture);
            break;
        case STATE_GAME_OVER:
            mainMenu.Update(program, fontTexture);
            break;
    }
}

void ProcessInput(const Uint8 * keys, GameState& gameState) {
    switch(mode) {
        case STATE_MAIN_MENU:
            if (keys[SDL_SCANCODE_SPACE]) {
                mode = STATE_GAME_LEVEL;
            }
            break;
        case STATE_GAME_LEVEL:
            gameState.ProcessInput(keys);
            if (keys[SDL_SCANCODE_Q]) {
                mode = STATE_MAIN_MENU;
                gameState.player.x = 0.15f;
                gameState.player.y = -0.3f;
                gameState.player.velX = 0.0f;
                gameState.player.velY = 0.0f;
            }
            break;
    }
    
}


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
    SpriteSheet tiles = SpriteSheet(LoadTexture(RESOURCE_FOLDER"arne_sprites.PNG"), 16, 8);
    SpriteSheet cuties = SpriteSheet(LoadTexture(RESOURCE_FOLDER"yooyoo.PNG"), 6, 4);
    GLuint fontTexture = LoadTexture(RESOURCE_FOLDER"font1.png");
    
    GameState test;
    test.map.Load(RESOURCE_FOLDER"level2.txt");
    test.Texture = tiles;
    test.program = program;
    
    Entity player;
    player = Entity(test.map.entities[0].x, test.map.entities[0].y + 1.0f, TILE_SIZE, TILE_SIZE, false);
    player.sheet = cuties;
    player.spriteIndex = 0;
    player.sprites = { 0 };
    
    test.player = player;
    
    
    SDL_Event event;
    bool done = false;
    mode = STATE_MAIN_MENU;
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
        }
        
        Render(program, fontTexture, elapsed, test);
        Update(program, elapsed, fontTexture, test);
        ProcessInput(keys, test);
        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}



