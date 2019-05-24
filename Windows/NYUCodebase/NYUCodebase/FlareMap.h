#ifndef FLAREMAP_H
#define FLAREMAP_H

#include <string> 
#include <vector>

struct FlareMapEntity {
	std::string type;
	float x;
	float y;
};

class FlareMap {
public:
	FlareMap(float tileSize_);
	~FlareMap();

	void Load(const std::string fileName);

	int mapWidth;
	int mapHeight;
	float tileSize; 
	unsigned int **mapData;
	std::vector<FlareMapEntity> entities;

private:

	bool ReadHeader(std::ifstream &stream);
	bool ReadLayerData(std::ifstream &stream);
	bool ReadEntityData(std::ifstream &stream);
	
};

#endif