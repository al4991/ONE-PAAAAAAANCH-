#ifndef HITBOX_H
#define HITBOX_H

#include "Entity.h"
#include "FlareMap.h"

struct Hitbox {
	Entity body;
	float timeAlive;
	bool direction;

	bool checkFulfill(FlareMap* map);
};

#endif