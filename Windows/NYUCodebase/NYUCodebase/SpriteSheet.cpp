#include "SpriteSheet.h"

SpriteSheet::SpriteSheet() {}

SpriteSheet::SpriteSheet(unsigned int textureID_, int x, int y) {
    textureID = textureID_;
    spriteCountX = x;
    spriteCountY = y;
}
