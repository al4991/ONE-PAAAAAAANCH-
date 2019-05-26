#include "GameState.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include <algorithm>


GameState::GameState(ShaderProgram& p) {
    program = p;
}

GameState::~GameState() {
    Mix_FreeMusic(background);
    Mix_FreeChunk(door);
}

void GameState::loadMusic() {
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    background = Mix_LoadMUS(RESOURCE_FOLDER"background.mp3");
    door = Mix_LoadWAV(RESOURCE_FOLDER"door.wav");
}

void GameState::backgroundMusic() {
    Mix_PlayMusic(background, -1);
    Mix_VolumeMusic(30);
}

void GameState::playSound() {
    Mix_PlayChannel(0, door, 0.5);
    Mix_VolumeMusic(25);
}

void GameState::Render() {
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

void GameState::RenderLevel(FlareMap& map) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    float xOffset = std::min(std::max(-player.x, ((float)map.mapWidth * -TILE_SIZE) + 1.777f), -1.777f);
    float yOffset = std::min(std::max(-player.y, ((float)map.mapHeight * TILE_SIZE)), 2.0f);
    viewMatrix = glm::translate(viewMatrix, glm::vec3(xOffset, yOffset, 0.0f));
    program.SetModelMatrix(modelMatrix);
    program.SetViewMatrix(viewMatrix);
    DrawTiles(map);
}

void GameState::RenderMenu() {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetViewMatrix(viewMatrix);
    program.SetModelMatrix(modelMatrix);
    DrawText("Oof", 0.25f, 0.0f, -0.7f, 0.5f);
    DrawText("the game", 0.1f, 0.0f, 0.0f, 0.45f);
    DrawText("PRESS SPACE TO START", 0.075f, 0.0f, -0.7125f, -0.5f);
    DrawText("PRESS Q to QUIT", 0.05f, 0.00001f, 1.0f, -0.9f);
    DrawText("By Mithila", 0.05f, 0.0f, -1.7f, -0.825f);
    DrawText("    and", 0.05f, 0.0f, -1.7f, -0.875f);
    DrawText("  Andrew", 0.05f, 0.0f, -1.7f, -0.925f);
}

void GameState::RenderGameOver() {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetViewMatrix(viewMatrix);
    program.SetModelMatrix(modelMatrix);
    DrawText("Ya lost, it's over", .15f, 0.0f, -1.2f, -0.0f);
    DrawText("Press Space to", 0.075f, 0.0f, -0.6125f, -0.5f);
    DrawText("go back to Menu", 0.075f, 0.0f, -0.6125f, -0.575f);
    DrawText("Ya ape", 0.05f, 0.0f, -1.7f, -0.825f);
}

void GameState::RenderWin() {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    glm::mat4 viewMatrix = glm::mat4(1.0f);
    program.SetViewMatrix(viewMatrix);
    program.SetModelMatrix(modelMatrix);
    DrawText("You win", 0.15f, 0.0f, -0.45f, 0.0f);
}

void GameState::Update(float elapsed) {
	animationElapsed += elapsed;
	if (animationElapsed > 1.0 / framesPerSecond) {
		player.spriteIndex++;
		victory.spriteIndex++;
		animationElapsed = 0.0f;
		if (player.spriteIndex > 2) {
			player.spriteIndex = 0;
		}
		if (victory.spriteIndex > 11) {
			victory.spriteIndex = 0;
		}

		for (Entity& i : annoying) {
			if (!i.isStatic) {
				i.spriteIndex++;
				if (i.spriteIndex > 1) {
					i.spriteIndex = 0;
				}
			}
		}
	}
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
                    if (player.y > i.y) {
                        player.velY = 1.0f; 
                    }
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
                    if (player.y > i.y) {
                        player.velY = 1.0f;
                    }
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

void GameState::ProcessInput(const Uint8 * keys) {
    if (keys[SDL_SCANCODE_Q]) {
        done = true; 
    }

    switch (mode) {
    case (STATE_MAIN_MENU):
    case (STATE_GAME_OVER):
    case (STATE_WIN):
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
            if (player.collidedBottom) {
                player.accX = 0.0f;
                player.spriteSet = 0;
            }
        }
        if (keys[SDL_SCANCODE_SPACE]) {
            if (player.collidedBottom) {
                player.velY = 1.0f;
            }
        }
        break;
    }

}

void GameState::ProcessEvent(SDL_Event event) {
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
                player.spriteSet = 2; 
            }
        }
        if (event.key.keysym.scancode == SDL_SCANCODE_D) {
            if (player.wallJump && player.collidedRight) {
                player.wallJump = player.collidedRight = false;
                player.wallJumpFrames = 0.0f; 
                player.velX = -TILE_SIZE * 20;
                player.accX = -0.5f; 
                player.velY = TILE_SIZE * 8;
                player.spriteSet = 1;

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

void GameState::DrawTiles(FlareMap& map) {
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

void GameState::DrawText(std::string text, float size, float spacing, float posx, float posy) {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, glm::vec3(posx, posy, 0.0f));
    program.SetModelMatrix(modelMatrix);
    float character_size = 1.0 / 16.0f;
    std::vector<float> vertexData;
    std::vector<float> texCoordData;
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

Entity GameState::placeEnemy(float x, float y) {
    Entity a = Entity(
        x, y,
        TILE_SIZE, TILE_SIZE, false
    );
    a.sheet = Texture;
    a.spriteIndex = 0;
    a.sprites = {60, 76 };
    a.isStatic = true;
    return a;
}

void GameState::SetEntities() {
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

void GameState::Load() {
    font = LoadTexture(RESOURCE_FOLDER"font1.png");
    Texture = SpriteSheet(LoadTexture(RESOURCE_FOLDER"arne_sprites.PNG"), 16, 8);
    PlayerSprites = SpriteSheet(LoadTexture(RESOURCE_FOLDER"yooyoo.PNG"), 6, 4);
    level1.Load(RESOURCE_FOLDER"level1.txt");
    level2.Load(RESOURCE_FOLDER"level2.txt");
    level3.Load(RESOURCE_FOLDER"level3.txt");
    loadMusic();
}
