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
                            mapData[y][x] = 0;
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
        float vertices[] = {
            -0.5f * TILE_SIZE , -0.5f * TILE_SIZE,
            0.5f * TILE_SIZE, 0.5f * TILE_SIZE,
            -0.5f * TILE_SIZE , 0.5f * TILE_SIZE,
            0.5f * TILE_SIZE, 0.5f * TILE_SIZE,
            -0.5f * TILE_SIZE, -0.5f * TILE_SIZE ,
            0.5f * TILE_SIZE , -0.5f * TILE_SIZE
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

    void movePlayer(float elapsed) {
        //        worldToTileCoordinates(player.x, player.y, &gridX, &gridY);
        velX = lerp(velX, 0.0f, elapsed * fricX);
        velY = lerp(velY, 0.0f, elapsed * fricY);
        velX += accX * elapsed;
        velY += accY * elapsed;
        velY += gravityY * elapsed;
        x += velX * elapsed;
        y += velY * elapsed;
    }

    void checkCollision(unsigned int **mapData) {
        int gridX; 
        int gridY; 
        worldToTileCoordinates(x, y, &gridX, &gridY); 

    }

};

struct GameState {
    SpriteSheet Texture;
    SpriteSheet FontTexture;
    FlareMap map = FlareMap();
    Entity player = Entity();
    ShaderProgram program;

    void Render(ShaderProgram& p, float elapsed) {
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f)); 
        glm::mat4 viewMatrix = glm::mat4(1.0f); 
        viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.x, -player.y, 0.0f)); 
        p.SetModelMatrix(modelMatrix); 
        p.SetViewMatrix(viewMatrix); 

        DrawTiles();
        player.Render(p);
        player.movePlayer(elapsed);
    }

    void DrawTiles() {
        std::vector<float> vertexData;
        std::vector<float> texCoordData;
        for (int y = 0; y < map.mapHeight; y++) {
            for (int x = 0; x < map.mapWidth; x++) {
                if (map.mapData[y][x] != 0) {
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


    bool checkCollision() {

        return false;
    }

};

//void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
//    float character_size = 1.0 / 16.0f;
//
//    std::vector<float> vertexData;
//    std::vector<float> texCoordData;
//
//    for (int i = 0; i < text.size(); i++) {
//        int spriteIndex = (int)text[i];
//
//        float texture_x = (float)(spriteIndex % 16) / 16.0f;
//        float texture_y = (float)(spriteIndex / 16) / 16.0f;
//
//
//        vertexData.insert(vertexData.end(), {
//            ((size + spacing) * i) + (-0.5f * size), 0.5f * size,
//            ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
//            ((size + spacing) * i) + (0.5f * size), 0.5f * size,
//            ((size + spacing) * i) + (0.5f * size), -0.5f * size,
//            ((size + spacing) * i) + (0.5f * size), 0.5f * size,
//            ((size + spacing) * i) + (-0.5f * size), -0.5f * size,
//            });
//        texCoordData.insert(texCoordData.end(), {
//            texture_x, texture_y,
//            texture_x, texture_y + character_size,
//            texture_x + character_size, texture_y,
//            texture_x + character_size, texture_y + character_size,
//            texture_x + character_size, texture_y,
//            texture_x, texture_y + character_size,
//            });
//    }
//
//    glUseProgram(program.programID);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
//    glEnableVertexAttribArray(program.positionAttribute);
//
//    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
//    glEnableVertexAttribArray(program.texCoordAttribute);
//
//    glBindTexture(GL_TEXTURE_2D, fontTexture);
//    glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
//
//    glDisableVertexAttribArray(program.positionAttribute);
//    glDisableVertexAttribArray(program.texCoordAttribute);
//
//}


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
    SpriteSheet cuties = SpriteSheet(LoadTexture(RESOURCE_FOLDER"cuties.PNG"), 6, 8);

    GameState test;
    test.map.Load(RESOURCE_FOLDER"level2.txt");
    test.Texture = tiles;
    test.program = program;

    Entity player;
    player = Entity(test.map.entities[0].x, test.map.entities[0].y, TILE_SIZE, TILE_SIZE, false);
    player.sheet = cuties;
    player.spriteIndex = 0;
    player.sprites = { 16 };

    test.player = player;

    float one = 0.0f;
    float two = 0.0f;

    SDL_Event event;
    bool done = false;
    while (!done) {
        glClear(GL_COLOR_BUFFER_BIT);/*
        viewMatrix = glm::mat4(1.0f);
        viewMatrix = glm::translate(viewMatrix, glm::vec3(one, two, 0.0f));*/
        //viewMatrix = glm::translate(viewMatrix, glm::vec3(-player.x, -player.y, 0.0f));


        const Uint8 *keys = SDL_GetKeyboardState(NULL);
        float ticks = (float)SDL_GetTicks() / 1000.0f;
        float elapsed = ticks - lastFrameTicks;
        lastFrameTicks = ticks;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
        if (keys[SDL_SCANCODE_A]) {
            one += 0.1f;
        }
        else if (keys[SDL_SCANCODE_D]) {
            one -= 0.1f;
        }
        else if (keys[SDL_SCANCODE_W]) {
            two -= 0.1f;
        }
        else if (keys[SDL_SCANCODE_S]) {
            two += 0.1f;
        }

        if (keys[SDL_SCANCODE_LEFT]) {
            test.player.velX = -1.25f;
            test.player.velY = 0.0f;
            //            test.player.accY = 0.0f;

        }
        if (keys[SDL_SCANCODE_RIGHT]) {
            test.player.velX = 1.25f;
            test.player.velY = 0.0f;
            //            test.player.accY = 0.0f;
        }
        if (keys[SDL_SCANCODE_UP]) {
            test.player.velY = 1.25f;
            test.player.velX = 0.0f;
            if (test.player.velY > 2.0f) {
                test.player.velY = 1.25f;
            }
            //            test.player.accX = 0.0f;
        }
        //        if(keys[SDL_SCANCODE_DOWN]) {
        //            test.player.velY = -0.75f;
        //            test.player.accY = -1.0f;
        //            test.player.velX = 0.0f;
        //            test.player.accX = 0.0f;
        //        }


        test.Render(program, elapsed);

        SDL_GL_SwapWindow(displayWindow);
    }

    SDL_Quit();
    return 0;
}


