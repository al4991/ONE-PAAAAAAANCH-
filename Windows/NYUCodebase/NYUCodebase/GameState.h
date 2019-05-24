#ifndef GAMESTATE_H
#define GAMESTATE_H

#ifdef _WINDOWS
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_image.h>
#include <SDL_mixer.h>


#ifdef _WINDOWS
#define RESOURCE_FOLDER ""
#else
#define RESOURCE_FOLDER "NYUCodebase.app/Contents/Resources/"
#endif
#define STB_IMAGE_IMPLEMENTATION

#include "SpriteSheet.h"
#include "Entity.h"
#include "helper.h"
#include "ShaderProgram.h"
#include "FlareMap.h"
#include "Hitbox.h"
#include <vector>

struct GameState {
	SpriteSheet Texture;
	SpriteSheet PlayerSprites;
	GLuint font;

	GameMode mode = STATE_MAIN_MENU;

	FlareMap level1 = FlareMap(TILE_SIZE);
	FlareMap level2 = FlareMap(TILE_SIZE);
	FlareMap level3 = FlareMap(TILE_SIZE);

	Entity player;
	std::vector<Entity> annoying;
	Entity victory;
	ShaderProgram program;
	Hitbox* hitbox = NULL;
	Mix_Music *background;
	Mix_Chunk *door;

	GameState(ShaderProgram& p);

	~GameState();

	void loadMusic();

	void backgroundMusic();

	void playSound();

	void Render();

	void RenderLevel(FlareMap& map);

	void RenderMenu();

	void RenderGameOver();

	void RenderWin();

	void Update(float elapsed);

	void ProcessInput(const Uint8 * keys);

	void ProcessEvent(SDL_Event event);

	void DrawTiles(FlareMap& map);

	void DrawText(std::string text, float size, float spacing, float posx, float posy);

	Entity placeEnemy(float x, float y);

	void SetEntities();

	void Load();

};


#endif 