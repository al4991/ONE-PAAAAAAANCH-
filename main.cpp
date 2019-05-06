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
	float velX = 0.0f; 
	float velY = 0.0f; 
	float accX = 0.0f; 
	float accY = 0.0f; 

	std::vector<SpriteSheet> sprites;

	bool collidedTop = false; 
	bool collidedBottom = false;
	bool collidedLeft = false; 
	bool collidedRight = false; 
	bool isStatic = true; 
}




// stuff to make use of the map 
#include <fstream> 
#include <string>
#include <iostream> 
#include <sstream> 
#include <vector> 

using namespace std; 

std::vector<Entity> yeet; 

void placeEntity(string type, float placeX, float placeY) { 
	// You should allocate an array or something to push to based on 
	// the type. isStatic should be set according to type as well
	Entity newEnt = new Entity(0, 0, TILE_SIZE, TILE_SIZE, true)
	newEnt.SpriteSheet = levelData[placeY][placeX]; 
	yeet.push_back()

}

bool readHeader(std::ifstream &stream) {
	string line; 
	mapWidth = -1;
	mapHeight = -1; 
	while (getline(stream, line)) {
		if (line == "") { break; }

		istringstream sStream(line); 
		string key, value; 
		getline(sStream, key, '='); 
		getline(sStream, value); 

		if (key == "width") {
			mapWidth = atoi(value.c_str()); 
		} else if (key == "height") {
			mapHeight = atoi(value.c_str()); 
		}
	}

	if (mapWidth == -1 || mapHeight == -1) {
		return false; 
	} else { 
		levelData = new unsigned char*[mapHeight]; 
		for (int i = 0; i < mapHeight; ++i) {
			levelData[i] = new unsigned char[mapWidth];
		}
		return true; 
	}
}

bool readLayerData(std::ifstream &stream) {
	string line;
	while (getline(stream, line)) {
		if (line =="") { break; }
		istringstream sStream(line); 
		string key, value; 
		getline(sStream, key, '=');
		getline(sStream, value); 
		if (key == "data") {
			for (int y = 0; y < mapHeight; y++) {
				getline(stream, line);
				istringstream lineStream(line); 
				string tile; 

				for (int x = 0; x < mapWidth; x++) {
					getline(lineStream, tile, ','); 
					unsigned char val = (unsigned char)atoi(tile.c_str()); 
					if (val > 0) { 
						levelData[y][x] = val - 1; 
					} else { 
						levelData[y][x] = 0; 
					}
				}
			}
		}
	}
	return true;
}

bool readEntityData(std::ifstream &stream) {
	string line; 
	string type; 

	while (getline(stream, line)) {
		if (line == "") { break; }

		istringstream sStream(line); 
		string key, value; 
		getline(sStream, key, '=');
		getline(sStream, value); 

		if (key == "type") {
			type = value; 
		} else if (key == "location") {
			istringstream lineStream(value); 
			string xPosition, yPosition; 
			getline(lineStream, xPosition, ','); 
			getline(lineStream, yPosition, ','); 

			float placeX = atoi(xPosition.c_str()) * TILE_SIZE; 
			float placeY = atoi(yPosition.c_str()) * TILE_SIZE; 

			placeEntity(type, placeX, placeY); 
		}
	}
}



void readFile() {
	ifstream infile(levelFile); 
	string line; 
	while (getline(infile, line)) { 
		if (line == "[header]") {
			if (!readHeader(infile)) { 
				return; 
			}
		} else if (line == "[layer]") { 
			readLayerData(infile); 
		} else if (line == "[ObjectsLayer]") { 
			readEntityData(infile); 
		}
	}
}

