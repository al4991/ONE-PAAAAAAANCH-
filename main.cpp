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
				newEntity.x = std::atoi(xPosition.c_str());
				newEntity.y = std::atoi(yPosition.c_str());
				entities.push_back(newEntity);

				// In the slides, x and y are multiplied by tilesize
			}
		}
		return true;
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

	GLuint spritesheet; 

	bool collidedTop = false;
	bool collidedBottom = false;
	bool collidedLeft = false;
	bool collidedRight = false;
	bool isStatic = true;

};

struct GameState {
	GLuint Texture;
	GLuint FontTexture;
	FlareMap map;
	Entity player;
	ShaderProgram program; 


	float TILE_SIZE = 0.1f; 
	int SPRITE_COUNT_X = 16; 
	int SPRITE_COUNT_Y = 8; 

	void DrawTiles() {
		std::vector<float> vertexData;
		std::vector<float> texCoordData;
		for (int y = 0; y < map.mapHeight; y++) {
			for (int x = 0; x < map.mapWidth; x++) {
				if (map.mapData[y][x] != 0) { 
					float u = (float)(((int)map.mapData[y][x]) % SPRITE_COUNT_X) / (float)SPRITE_COUNT_X;
					float v = (float)(((int)map.mapData[y][x]) / SPRITE_COUNT_X) / (float)SPRITE_COUNT_Y;
					float spriteWidth = 1.0f / (float)SPRITE_COUNT_X;
					float spriteHeight = 1.0f / (float)SPRITE_COUNT_Y;
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

		glBindTexture(GL_TEXTURE_2D, Texture);
		glDrawArrays(GL_TRIANGLES, 0, vertexData.size()/2);

		glDisableVertexAttribArray(program.positionAttribute);
		glDisableVertexAttribArray(program.texCoordAttribute);
	}


};
//
//
//void DrawText(ShaderProgram &program, int fontTexture, std::string text, float size, float spacing) {
//	float character_size = 1.0 / 16.0f;
//
//	std::vector<float> vertexData;
//	std::vector<float> texCoordData;
//
//	for (int i = 0; i < text.size(); i++) {
//		int spriteIndex = (int)text[i];
//
//		float texture_x = (float)(spriteIndex % 16) / 16.0f;
//		float texture_y = (float)(spriteIndex / 16) / 16.0f;
//
//
//		vertexData.insert(vertexData.end(), {
//			((size + spacing) * i) + (-0.5f * size), 0.5f * size,
//			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
//			((size + spacing) * i) + (0.5f * size), 0.5f * size,
//			((size + spacing) * i) + (0.5f * size), -0.5f * size,
//			((size + spacing) * i) + (0.5f * size), 0.5f * size,
//			((size + spacing) * i) + (-0.5f * size), -0.5f * size,
//			});
//		texCoordData.insert(texCoordData.end(), {
//			texture_x, texture_y,
//			texture_x, texture_y + character_size,
//			texture_x + character_size, texture_y,
//			texture_x + character_size, texture_y + character_size,
//			texture_x + character_size, texture_y,
//			texture_x, texture_y + character_size,
//			});
//	}
//
//	glUseProgram(program.programID);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//	glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertexData.data());
//	glEnableVertexAttribArray(program.positionAttribute);
//
//	glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texCoordData.data());
//	glEnableVertexAttribArray(program.texCoordAttribute);
//
//	glBindTexture(GL_TEXTURE_2D, fontTexture);
//	glDrawArrays(GL_TRIANGLES, 0, text.size() * 6);
//
//	glDisableVertexAttribArray(program.positionAttribute);
//	glDisableVertexAttribArray(program.texCoordAttribute);
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

	FlareMap map = FlareMap();
	map.Load("level2.txt");



	glm::mat4 projectionMatrix = glm::mat4(1.0f);
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	glm::mat4 viewMatrix = glm::mat4(1.0f);
	projectionMatrix = glm::ortho(-1.777f, 1.777f, -1.0f, 1.0f, -1.0f, 1.0f);

	glUseProgram(program.programID);

	GameState doggo;
	doggo.map = FlareMap();
	doggo.map.Load("level2.txt"); 
	doggo.Texture = LoadTexture(RESOURCE_FOLDER"arne_sprites.PNG");
	doggo.program = program;
	doggo.player = Entity(doggo.map.entities[0].x, doggo.map.entities[0].y, 0.1f, 0.1f, false); 
	doggo.player.spritesheet = LoadTexture(RESOURCE_FOLDER"cuties.PNG"); 

	for (FlareMapEntity i : doggo.map.entities) {
		std::cout << i.type << std::endl; 
	}
	std::cout << "HELLO ===============================" << std::endl; 

	float yah = 0.0f; 
	float yeet = 0.0f; 
    SDL_Event event;
    bool done = false;
    while (!done) {
		glClear(GL_COLOR_BUFFER_BIT);
		viewMatrix = glm::mat4(1.0f);
		viewMatrix = glm::translate(viewMatrix, glm::vec3(yah, yeet, 0.0f));
		const Uint8 *keys = SDL_GetKeyboardState(NULL);
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                done = true;
            }
        }
		if (keys[SDL_SCANCODE_A]) {
			yah += 0.1f; 
		}
		else if (keys[SDL_SCANCODE_D]) {
			yah -= 0.1f; 
		}else if (keys[SDL_SCANCODE_W]) {
			yeet += 0.1f; 
		}
		else if (keys[SDL_SCANCODE_S]) {
			yeet -= 0.1f; 
		}
		program.SetModelMatrix(modelMatrix); 
		program.SetProjectionMatrix(projectionMatrix);
		program.SetViewMatrix(viewMatrix);        
		doggo.DrawTiles(); 

        SDL_GL_SwapWindow(displayWindow);
    }
    
    SDL_Quit();
    return 0;
}
