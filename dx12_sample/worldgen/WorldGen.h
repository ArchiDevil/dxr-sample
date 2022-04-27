#pragma once

#include "Noise.h"

#include <cmath>
#include <fstream>
#include <string>

class WorldGen
{
public:
    WorldGen() = default;

    void GenerateHeightMap(int size);
    int  GetHeight(int ChunkX, int ChunkY);

private:
    Noise noise;

    static const int _worldSize = 1024;
    int              _heightMap[_worldSize][_worldSize] = {};
};
