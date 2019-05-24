#ifndef SPRITESHEET_H
#define SPRITESHEET_H

struct SpriteSheet {
	unsigned int textureID;
	int spriteCountX;
	int spriteCountY;

	SpriteSheet();
	SpriteSheet(unsigned int textureID_, int x, int y);

};

#endif