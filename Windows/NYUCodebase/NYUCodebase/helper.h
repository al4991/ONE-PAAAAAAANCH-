#pragma once 
#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#include <SDL.h>
#include <SDL_opengl.h>
#include <vector>

#define FIXED_TIMESTEP 0.0166666f
#define MAX_TIMESTEPS 6
#define TILE_SIZE 0.1f

extern bool done; 
extern float lastFrameTicks;
extern float accumulator;
extern float animationElapsed;
extern float framesPerSecond;
extern std::vector<int> solidTiles;
extern std::vector<int> dangerTiles;
extern std::vector<int> climbTile;
enum GameMode { STATE_MAIN_MENU, STATE_GAME_LEVEL1, STATE_GAME_LEVEL2, STATE_GAME_LEVEL3, STATE_GAME_OVER, STATE_WIN };


float lerp(float v0, float v1, float t);

GLuint LoadTexture(const char *filePath);

void worldToTileCoordinates(float worldX, float worldY, int *gridX, int *gridY);

