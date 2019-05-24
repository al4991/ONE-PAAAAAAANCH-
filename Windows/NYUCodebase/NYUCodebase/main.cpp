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

#include "ShaderProgram.h"
#include "FlareMap.h"
#include "SpriteSheet.h"
#include "Hitbox.h"
#include "Entity.h"
#include "GameState.h"
#include "helper.h"

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

SDL_Window* displayWindow;


int main(int argc, char *argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	displayWindow = SDL_CreateWindow("Drink milk is good for you", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_OPENGL);
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

			for (Entity& i : game.annoying) {
				if (!i.isStatic) {
					i.spriteIndex++; 
					if (i.spriteIndex > 1) {
						i.spriteIndex = 0; 
					}
				}
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




