#ifndef ENTITY_H
#define ENTITY_H

#include "SpriteSheet.h"
#include "FlareMap.h"
#include "ShaderProgram.h"
#include <vector>

class Entity {
public:
	Entity();
	Entity(float x_, float y_, float width_, float height_, bool isStatic_);

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

	void Render(ShaderProgram& p);

	bool Update(float elapsed, FlareMap* map);

	void resolveCollisionX(Entity& entity);

	void resolveCollisionY(Entity& entity);

	bool CollidesWith(Entity& entity);

	bool checkTileCollision(FlareMap *map);
};

#endif 